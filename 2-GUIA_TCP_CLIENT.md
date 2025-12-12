# Guia: Cliente TCP con nRF9151 DK

## Objetivo

Crear una aplicacion que:
1. Se conecte a la red LTE (celular)
2. Establezca conexion TCP con servidor en **72.60.48.208:5555**
3. Envie **"hola mundo"** cada segundo
4. Reciba **"HOLA MUNDO"** como respuesta

---

## Requisitos Previos

- nRF9151 DK con tarjeta SIM activa (Onomondo o Wireless Logic)
- nRF Connect SDK v3.2.0 instalado
- VS Code con nRF Connect Extension Pack
- Antena LTE conectada o usar la integrada

---

## Paso 1: Crear el Proyecto

1. En VS Code, panel **nRF Connect**
2. Click en **"Create a new application"**
3. Seleccionar **"Create blank application"**
4. Nombre: `tcp_client`
5. Ubicacion: `C:\Users\franc\Desktop\Repos\EMIC\nRF9151_DK\tcp_client`

---

## Paso 2: Estructura del Proyecto

```
tcp_client/
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
project(tcp_client)

target_sources(app PRIVATE src/main.c)
```

### 3.2 prj.conf

```conf
#
# Configuracion TCP Client para nRF9151
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

menu "TCP Client Configuration"

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
 * TCP Client para nRF9151 DK
 *
 * Conecta a servidor TCP, envia "hola mundo" cada segundo
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
#define TX_MESSAGE      "hola mundo"
#define RX_BUFFER_SIZE  64

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
        if (evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ||
            evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_ROAMING) {
            printk("LTE conectado: %s\n",
                   evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
                   "Home" : "Roaming");
            k_sem_give(&lte_connected_sem);
        }
        break;
    case LTE_LC_EVT_RRC_UPDATE:
        printk("RRC modo: %s\n",
               evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
               "Connected" : "Idle");
        break;
    default:
        break;
    }
}

/*
 * Inicializar modem y conectar a red LTE
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

    printk("Conectando a red LTE...\n");

    err = lte_lc_connect_async(lte_handler);
    if (err) {
        printk("Error conectando a LTE: %d\n", err);
        return err;
    }

    /* Esperar conexion LTE (timeout 120 segundos) */
    if (k_sem_take(&lte_connected_sem, K_SECONDS(120)) != 0) {
        printk("Timeout esperando conexion LTE\n");
        return -ETIMEDOUT;
    }

    printk("Conexion LTE establecida!\n");
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

    /* Recibir respuesta (con timeout) */
    struct timeval timeout = {
        .tv_sec = 5,
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
    printk("   TCP Client - nRF9151 DK\n");
    printk("   Servidor: %s:%d\n", SERVER_IP, SERVER_PORT);
    printk("========================================\n\n");

    /* Inicializar modem y conectar LTE */
    err = modem_init_and_connect();
    if (err) {
        printk("Error fatal: No se pudo conectar a LTE\n");
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
                k_sleep(K_SECONDS(30));
                retry_count = 0;
            } else {
                printk("Reintentando en 5 segundos... (%d/%d)\n",
                       retry_count, max_retries);
                k_sleep(K_SECONDS(5));
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

   > **Importante**: Para aplicaciones que usan el modem LTE, usar la variante `ns` (Non-Secure) porque el modem corre en el dominio seguro.

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
*** Booting nRF Connect SDK v3.2.0-5dcc6bd39b0f ***
*** Using Zephyr OS v4.2.99-a57ad913cf4e ***

========================================
   TCP Client - nRF9151 DK
   Servidor: 72.60.48.208:5555
========================================

Inicializando modem...
Conectando a red LTE...
RRC modo: Connected
LTE conectado: Roaming
Conexion LTE establecida!
Creando socket TCP...
Conectando a 72.60.48.208:5555...
Conexion TCP establecida!
TX: hola mundo
RX: Connected to Uppercase Server. Send text and it will be returned in UPPERCASE.
TX: hola mundo
RX: HOLA MUNDO
TX: hola mundo
RX: HOLA MUNDO
...
```

> **Nota**: El servidor 72.60.48.208:5555 es un "Uppercase Server" que convierte el texto recibido a mayusculas.

---

## Solucion de Problemas

### No conecta a LTE

1. Verificar que la SIM esta bien insertada
2. Verificar cobertura LTE-M/NB-IoT en tu zona
3. La antena LTE debe estar conectada
4. Esperar mas tiempo (puede tomar 1-2 minutos la primera vez)

### Error de conexion TCP

1. Verificar que el servidor esta activo en 72.60.48.208:5555
2. Verificar conectividad de red
3. Revisar firewall del servidor

### No recibe respuesta

1. Verificar que el servidor responde correctamente
2. Aumentar timeout en el codigo si es necesario

---

## Diagrama de Flujo

```
┌─────────────────┐
│     Inicio      │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Iniciar Modem   │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Conectar LTE    │◄────────┐
└────────┬────────┘         │
         │                  │
         ▼                  │
┌─────────────────┐         │
│ Conectar TCP    │─────────┤ Error
│ 72.60.48.208    │         │
└────────┬────────┘         │
         │                  │
         ▼                  │
┌─────────────────┐         │
│ Enviar          │         │
│ "hola mundo"    │         │
└────────┬────────┘         │
         │                  │
         ▼                  │
┌─────────────────┐         │
│ Recibir         │         │
│ "HOLA MUNDO"    │─────────┘
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Esperar 1 seg   │
└────────┬────────┘
         │
         └──────► (repetir)
```

---

## Recursos

- [Nordic Semiconductor - Cellular Samples](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/samples/cellular.html)
- [Zephyr BSD Sockets](https://docs.zephyrproject.org/latest/connectivity/networking/api/sockets.html)
- [LTE Link Control](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/libraries/modem/lte_lc.html)
- [nRF Modem Library](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/libraries/modem/nrf_modem_lib/nrf_modem_lib.html)

---

## Resultados de Prueba

**Fecha:** 12 de Diciembre 2025

| Parametro | Resultado |
|-----------|-----------|
| Hardware | nRF9151 DK |
| SDK | nRF Connect SDK v3.2.0 |
| Zephyr OS | v4.2.99 |
| SIM | Onomondo (incluida con el kit) |
| Conexion LTE | Roaming |
| Servidor TCP | 72.60.48.208:5555 |
| TX | "hola mundo" (cada 1 segundo) |
| RX | "HOLA MUNDO" |
| Estado | **EXITOSO** |

---

*Guia creada para nRF9151 DK con nRF Connect SDK v3.2.0*
