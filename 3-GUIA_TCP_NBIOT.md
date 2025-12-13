# Guia: Cliente TCP con NB-IoT en nRF9151 DK

## Objetivo

Crear una aplicacion que:
1. Se conecte a la red **NB-IoT** (Narrowband IoT)
2. Establezca conexion TCP con servidor en **72.60.48.208:5555**
3. Envie **"hola mundo"** cada segundo
4. Reciba **"HOLA MUNDO"** como respuesta

---

## Diferencia entre LTE-M y NB-IoT

| Caracteristica | LTE-M | NB-IoT |
|----------------|-------|--------|
| Ancho de banda | 1.4 MHz | 200 kHz |
| Velocidad max | 1 Mbps | 250 kbps |
| Latencia | Baja (~10ms) | Mayor (~1-10s) |
| Movilidad | Soportada | Limitada |
| Cobertura | Buena | Excelente (penetracion) |
| Consumo | Bajo | Muy bajo |
| Uso tipico | Trackers, wearables | Sensores, medidores |

**NB-IoT** es ideal para:
- Dispositivos estaticos (medidores, sensores)
- Ubicaciones con mala cobertura (sotanos, zonas rurales)
- Aplicaciones que toleran mayor latencia
- Maximo ahorro de bateria

---

## Requisitos Previos

- nRF9151 DK con tarjeta SIM activa (Onomondo o Wireless Logic)
- nRF Connect SDK v3.2.0 instalado
- VS Code con nRF Connect Extension Pack
- Cobertura NB-IoT en tu zona (verificar con tu operador)

---

## Paso 1: Crear el Proyecto

1. En VS Code, panel **nRF Connect**
2. Click en **"Create a new application"**
3. Seleccionar **"Create blank application"**
4. Nombre: `tcp_client_nbiot`
5. Ubicacion: `C:\Users\franc\Desktop\Repos\EMIC\nRF9151_DK\tcp_client_nbiot`

---

## Paso 2: Estructura del Proyecto

```
tcp_client_nbiot/
├── CMakeLists.txt
├── prj.conf
├── Kconfig
└── src/
    └── main.c
```

---

## Paso 3: Archivos del Proyecto

### 3.1 CMakeLists.txt

```cmake
#
# Copyright (c) 2024 EMIC
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tcp_client_nbiot)

target_sources(app PRIVATE src/main.c)
```

### 3.2 prj.conf

```conf
#
# Configuracion TCP Client NB-IoT para nRF9151
#

# General
CONFIG_NCS_SAMPLES_DEFAULTS=y
CONFIG_SERIAL=y
CONFIG_CONSOLE=y
CONFIG_UART_CONSOLE=y

# Network
CONFIG_NETWORKING=y
CONFIG_NET_NATIVE=n
CONFIG_NET_SOCKETS=y
CONFIG_NET_SOCKETS_OFFLOAD=y
CONFIG_POSIX_API=y

# LTE Link Control
CONFIG_LTE_LINK_CONTROL=y

# ============================================
# CONFIGURACION NB-IoT (diferencia principal)
# ============================================
CONFIG_LTE_NETWORK_MODE_NBIOT=y

# Timeout mas largo para NB-IoT (conexion mas lenta)
CONFIG_LTE_NETWORK_TIMEOUT=300

# Modem Library
CONFIG_NRF_MODEM_LIB=y

# Heap y Stacks
CONFIG_HEAP_MEM_POOL_SIZE=4096
CONFIG_MAIN_STACK_SIZE=8192
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=2048

# Logs
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=3

# Configuracion del servidor TCP
CONFIG_TCP_SERVER_ADDRESS="72.60.48.208"
CONFIG_TCP_SERVER_PORT=5555
CONFIG_TCP_SEND_INTERVAL_SECONDS=1
```

### 3.3 Kconfig

```kconfig
#
# Copyright (c) 2024 EMIC
# SPDX-License-Identifier: Apache-2.0
#

menu "TCP Client NB-IoT Configuration"

config TCP_SERVER_ADDRESS
	string "Direccion IP del servidor TCP"
	default "72.60.48.208"

config TCP_SERVER_PORT
	int "Puerto del servidor TCP"
	default 5555

config TCP_SEND_INTERVAL_SECONDS
	int "Intervalo de envio en segundos"
	default 1

endmenu

source "Kconfig.zephyr"
```

### 3.4 src/main.c

```c
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
```

---

## Paso 4: Compilar

1. En VS Code, panel **nRF Connect**, click en **"Add Build Configuration"**
2. Seleccionar board: **nrf9151dk/nrf9151/ns** (Non-Secure)
3. Click en **"Build Configuration"**

### Comando alternativo (terminal):

```bash
west build -b nrf9151dk/nrf9151/ns
```

---

## Paso 5: Flashear

1. Conectar el nRF9151 DK via USB
2. Insertar la tarjeta SIM (Onomondo o Wireless Logic)
3. Click en **Flash** en el panel nRF Connect

### Comando alternativo:

```bash
west flash
```

---

## Paso 6: Monitorear

1. Abrir terminal serial en **COM11** a **115200 baud**
2. Deberias ver:

```
*** Booting nRF Connect SDK v3.2.0 ***
*** Using Zephyr OS v4.2.99 ***

========================================
   TCP Client NB-IoT - nRF9151 DK
   Servidor: 72.60.48.208:5555
   Modo: NB-IoT
========================================

Inicializando modem...
Conectando a red NB-IoT...
(NB-IoT puede tardar mas que LTE-M, espere...)
NB-IoT: Buscando red...
RRC modo: Connected
NB-IoT conectado: Roaming
Conexion NB-IoT establecida!
Creando socket TCP...
Conectando a 72.60.48.208:5555...
Conexion TCP establecida!
TX: hola mundo (NB-IoT)
RX: Connected to Uppercase Server. Send text and it will be returned in UPPERCASE.
TX: hola mundo (NB-IoT)
RX: HOLA MUNDO (NB-IOT)
...
```

> **Nota**: La conexion NB-IoT puede tardar entre 30 segundos y varios minutos dependiendo de la cobertura.

---

## Diferencias con el Proyecto LTE-M

| Aspecto | tcp_client (LTE-M) | tcp_client_nbiot |
|---------|-------------------|------------------|
| Modo de red | Por defecto (LTE-M) | `CONFIG_LTE_NETWORK_MODE_NBIOT=y` |
| Timeout conexion | 120 segundos | 300 segundos |
| Timeout recepcion | 5 segundos | 30 segundos |
| Tiempo entre reintentos | 5 segundos | 10 segundos |
| Mensaje TX | "hola mundo" | "hola mundo (NB-IoT)" |

---

## Solucion de Problemas

### No conecta a NB-IoT

1. **Verificar cobertura**: NB-IoT no esta disponible en todas las zonas
   - Consultar mapa de cobertura de tu operador
   - Algunos operadores solo ofrecen LTE-M

2. **La SIM debe soportar NB-IoT**:
   - No todas las SIM tienen NB-IoT habilitado
   - Verificar con el proveedor de la SIM

3. **Aumentar timeout**: Si la senal es debil, aumentar `CONFIG_LTE_NETWORK_TIMEOUT`

4. **Probar modo dual**: Cambiar a `CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT=y` para permitir fallback a LTE-M

### Conexion muy lenta

NB-IoT tiene mayor latencia que LTE-M. Esto es normal:
- Tiempo de conexion: 30s - 5min
- Latencia de datos: 1-10 segundos

### Error de conexion TCP

1. Verificar que el servidor esta activo
2. NB-IoT puede tener restricciones de puertos en algunos operadores
3. Algunos operadores bloquean trafico TCP en NB-IoT (solo permiten UDP/CoAP)

---

## Configuraciones Alternativas

### Solo NB-IoT (actual)
```conf
CONFIG_LTE_NETWORK_MODE_NBIOT=y
```

### NB-IoT con GPS
```conf
CONFIG_LTE_NETWORK_MODE_NBIOT_GPS=y
```

### Modo dual LTE-M/NB-IoT (fallback automatico)
```conf
CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT=y
```

### Preferir NB-IoT pero permitir LTE-M
```conf
CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT=y
CONFIG_LTE_MODE_PREFERENCE=2
```

---

## Resultados de Prueba

**Fecha:** 12 de Diciembre 2025
**Ubicacion:** Buenos Aires, Argentina

| Parametro | Resultado |
|-----------|-----------|
| Hardware | nRF9151 DK |
| SDK | nRF Connect SDK v3.2.0 |
| Zephyr OS | v4.2.99 |
| Servidor TCP | 72.60.48.208:5555 |
| Estado | **PENDIENTE** |

### Intentos de conexion NB-IoT

| SIM | Operador | Resultado | Error |
|-----|----------|-----------|-------|
| Onomondo | Multi-operador | Rechazado | EMM cause 15, 19 |
| Conexa | - | Rechazado | EMM cause 15 |
| Personal | Telecom Argentina | Rechazado | EMM cause 15 |

### Codigos de error EMM

| Codigo | Significado |
|--------|-------------|
| EMM cause 15 | No suitable cells in tracking area |
| EMM cause 19 | ESM failure (fallo servicio de datos) |

### Conclusion preliminar

En la ubicacion de prueba (Buenos Aires, Argentina) no se logro conexion NB-IoT con ninguna de las SIMs probadas. Posibles causas:

1. **Cobertura NB-IoT limitada** en Argentina - Los operadores han desplegado principalmente LTE-M
2. **SIMs no habilitadas** para NB-IoT - Requieren planes especificos para IoT
3. **Zona sin cobertura** - NB-IoT puede estar disponible en otras areas

### Proximos pasos

- [ ] Probar en otra ubicacion con cobertura NB-IoT confirmada
- [ ] Contactar operador para verificar disponibilidad NB-IoT
- [ ] Probar con SIM IoT especifica con NB-IoT habilitado

> **Nota**: El codigo compila y flashea correctamente. La prueba funcional queda pendiente para una ubicacion con cobertura NB-IoT disponible.

---

## Recursos

- [LTE Link Control - Nordic](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/libraries/modem/lte_lc.html)
- [Cellular IoT Fundamentals - Nordic Academy](https://academy.nordicsemi.com/courses/cellular-iot-fundamentals/)
- [NB-IoT vs LTE-M Comparison](https://www.nordicsemi.com/Products/Low-power-cellular-IoT)

---

*Guia creada para nRF9151 DK con nRF Connect SDK v3.2.0*
