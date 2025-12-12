/*
 * TCP Client NB-IoT para nRF9151 DK
 *
 * Conecta a servidor TCP usando NB-IoT, envia "hola mundo" cada segundo
 * y recibe "HOLA MUNDO" como respuesta.
 *
 * Copyright (c) 2024 EMIC
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>

/* Configuracion del servidor */
#define SERVER_IP       CONFIG_TCP_SERVER_ADDRESS
#define SERVER_PORT     CONFIG_TCP_SERVER_PORT
#define SEND_INTERVAL   CONFIG_TCP_SEND_INTERVAL_SECONDS

/* Mensaje a enviar */
#define TX_MESSAGE      "hola mundo (NB-IoT)"
#define RX_BUFFER_SIZE  128

/* Variables globales */
static int client_fd = -1;
static struct sockaddr_in server_addr;

/* Semaforo para esperar conexion LTE */
K_SEM_DEFINE(lte_connected_sem, 0, 1);

/*
 * Handler de eventos LTE
 */
static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		switch (evt->nw_reg_status) {
		case LTE_LC_NW_REG_REGISTERED_HOME:
			printk("NB-IoT conectado: Home\n");
			k_sem_give(&lte_connected_sem);
			break;
		case LTE_LC_NW_REG_REGISTERED_ROAMING:
			printk("NB-IoT conectado: Roaming\n");
			k_sem_give(&lte_connected_sem);
			break;
		case LTE_LC_NW_REG_SEARCHING:
			printk("NB-IoT: Buscando red...\n");
			break;
		case LTE_LC_NW_REG_REGISTRATION_DENIED:
			printk("NB-IoT: Registro denegado\n");
			break;
		case LTE_LC_NW_REG_NOT_REGISTERED:
			printk("NB-IoT: No registrado\n");
			break;
		default:
			break;
		}
		break;
	case LTE_LC_EVT_RRC_UPDATE:
		printk("RRC modo: %s\n",
		       evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
		       "Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		printk("Celda: ID=%d, TAC=%d\n",
		       evt->cell.id, evt->cell.tac);
		break;
	default:
		break;
	}
}

/*
 * Inicializar modem y conectar a red NB-IoT
 */
static int modem_init_and_connect(void)
{
	int err;

	printk("Inicializando modem...\n");

	err = nrf_modem_lib_init();
	if (err) {
		printk("Error inicializando modem: %d\n", err);
		return err;
	}

	printk("Conectando a red NB-IoT...\n");
	printk("(NB-IoT puede tardar mas que LTE-M, espere...)\n");

	err = lte_lc_connect_async(lte_handler);
	if (err) {
		printk("Error conectando a NB-IoT: %d\n", err);
		return err;
	}

	/* Esperar conexion NB-IoT (timeout 300 segundos - mas largo que LTE-M) */
	if (k_sem_take(&lte_connected_sem, K_SECONDS(300)) != 0) {
		printk("Timeout esperando conexion NB-IoT\n");
		printk("Verifique cobertura NB-IoT en su zona\n");
		return -ETIMEDOUT;
	}

	printk("Conexion NB-IoT establecida!\n");
	return 0;
}

/*
 * Conectar al servidor TCP
 */
static int tcp_connect(void)
{
	int err;

	printk("Creando socket TCP...\n");

	/* Crear socket TCP */
	client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client_fd < 0) {
		printk("Error creando socket: %d\n", errno);
		return -errno;
	}

	/* Configurar direccion del servidor */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);

	err = inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
	if (err != 1) {
		printk("Error convirtiendo IP: %s\n", SERVER_IP);
		close(client_fd);
		return -EINVAL;
	}

	printk("Conectando a %s:%d...\n", SERVER_IP, SERVER_PORT);

	/* Conectar al servidor */
	err = connect(client_fd, (struct sockaddr *)&server_addr,
		      sizeof(server_addr));
	if (err < 0) {
		printk("Error conectando al servidor: %d\n", errno);
		close(client_fd);
		return -errno;
	}

	printk("Conexion TCP establecida!\n");
	return 0;
}

/*
 * Enviar mensaje y recibir respuesta
 */
static int tcp_send_receive(void)
{
	int err;
	char rx_buffer[RX_BUFFER_SIZE];

	/* Enviar mensaje */
	printk("TX: %s\n", TX_MESSAGE);

	err = send(client_fd, TX_MESSAGE, strlen(TX_MESSAGE), 0);
	if (err < 0) {
		printk("Error enviando: %d\n", errno);
		return -errno;
	}

	/* Recibir respuesta (con timeout mas largo para NB-IoT) */
	struct timeval timeout = {
		.tv_sec = 30,  /* Timeout mas largo para NB-IoT */
		.tv_usec = 0
	};

	err = setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO,
			 &timeout, sizeof(timeout));
	if (err < 0) {
		printk("Error configurando timeout: %d\n", errno);
	}

	memset(rx_buffer, 0, sizeof(rx_buffer));
	err = recv(client_fd, rx_buffer, sizeof(rx_buffer) - 1, 0);

	if (err > 0) {
		printk("RX: %s\n", rx_buffer);
	} else if (err == 0) {
		printk("Conexion cerrada por servidor\n");
		return -ECONNRESET;
	} else {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			printk("Timeout esperando respuesta\n");
		} else {
			printk("Error recibiendo: %d\n", errno);
			return -errno;
		}
	}

	return 0;
}

/*
 * Cerrar conexion TCP
 */
static void tcp_disconnect(void)
{
	if (client_fd >= 0) {
		close(client_fd);
		client_fd = -1;
		printk("Socket cerrado\n");
	}
}

/*
 * Funcion principal
 */
int main(void)
{
	int err;
	int retry_count = 0;
	const int max_retries = 3;

	printk("\n");
	printk("========================================\n");
	printk("   TCP Client NB-IoT - nRF9151 DK\n");
	printk("   Servidor: %s:%d\n", SERVER_IP, SERVER_PORT);
	printk("   Modo: NB-IoT\n");
	printk("========================================\n\n");

	/* Inicializar modem y conectar NB-IoT */
	err = modem_init_and_connect();
	if (err) {
		printk("Error fatal: No se pudo conectar a NB-IoT\n");
		return -1;
	}

	/* Bucle principal con reconexion */
	while (1) {
		/* Conectar al servidor TCP */
		err = tcp_connect();
		if (err) {
			retry_count++;
			if (retry_count >= max_retries) {
				printk("Maximo de reintentos alcanzado\n");
				k_sleep(K_SECONDS(60));  /* Espera mas larga para NB-IoT */
				retry_count = 0;
			} else {
				printk("Reintentando en 10 segundos... (%d/%d)\n",
				       retry_count, max_retries);
				k_sleep(K_SECONDS(10));
			}
			continue;
		}

		retry_count = 0;

		/* Bucle de envio/recepcion */
		while (1) {
			err = tcp_send_receive();
			if (err) {
				printk("Error en comunicacion, reconectando...\n");
				tcp_disconnect();
				break;
			}

			/* Esperar intervalo antes del siguiente envio */
			k_sleep(K_SECONDS(SEND_INTERVAL));
		}
	}

	return 0;
}
