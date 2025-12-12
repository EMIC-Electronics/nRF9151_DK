# Guia de Inicio - nRF9151 DK

## Introduccion

El **nRF9151 DK** (PCA10171) es un kit de desarrollo para evaluacion y desarrollo con el System-in-Package (SiP) nRF9151, que soporta:

- **LTE-M / NB-IoT** - Conectividad celular IoT
- **GNSS** - Posicionamiento global
- **DECT NR+** - Comunicacion inalambrica

### Caracteristicas del Hardware

| Caracteristica | Especificacion |
|----------------|----------------|
| Procesador | ARM Cortex-M33 @ 64 MHz |
| Memoria Flash | 1 MB |
| RAM | 256 KB |
| Seguridad | ARM TrustZone + CryptoCell 310 |
| LEDs programables | 4 |
| Botones programables | 4 |
| Alimentacion | 3.0-5.5V externa o 5V USB |
| Programador | SEGGER J-Link OB integrado |

El kit incluye tarjetas SIM de **Onomondo** y **Wireless Logic** con datos gratuitos precargados.

---

## Paso 1: Requisitos de Software

### 1.1 Instalar Visual Studio Code

1. Descargar VS Code desde: https://code.visualstudio.com/download
2. Instalar la version correspondiente a tu sistema operativo (Windows/macOS/Linux)

### 1.2 Instalar nRF Connect Extension Pack

1. Abrir VS Code
2. Ir a la barra lateral y hacer clic en el icono de **Extensions** (o presionar `Ctrl+Shift+X`)
3. Buscar: **"nRF Connect for VS Code Extension Pack"**
4. Hacer clic en **Install**

> **Importante:** Instalar el **Extension Pack** completo, no solo "nRF Connect for VS Code"

### 1.3 Instalar el nRF Connect SDK

1. En VS Code, buscar el panel **nRF Connect** en la barra lateral izquierda
2. En la seccion **WELCOME**, hacer clic en **Install SDK**
3. Seleccionar **"nRF Connect SDK"** (no "Bare Metal")
4. Elegir la version mas reciente estable (ej: v2.9.0 o superior)
5. Seleccionar una carpeta de instalacion (ej: `C:\ncs`)

### 1.4 Instalar el Toolchain

1. En el panel nRF Connect, ir a **Manage Toolchains**
2. Instalar el toolchain correspondiente a la version del SDK instalado
3. Esperar a que se complete la descarga e instalacion

---

## Paso 2: Conectar el Hardware

### 2.1 Conexion Fisica

1. **Insertar la tarjeta SIM** (opcional para este ejemplo de LED)
   - Usar la ranura nano/4FF SIM incluida en el kit
   - El kit viene con SIMs de Onomondo y Wireless Logic

2. **Conectar el cable USB**
   - Conectar el cable micro-USB al puerto **J4** (etiquetado como "DEBUG")
   - Conectar el otro extremo al PC

3. **Verificar alimentacion**
   - El LED de encendido debe iluminarse
   - El switch **SW9** debe estar en posicion "ON"

### 2.2 Verificar la Conexion

1. Abrir **Administrador de Dispositivos** (Windows)
2. Buscar en "Puertos (COM y LPT)":
   - Deberia aparecer **"JLink CDC UART Port (COMx)"**
3. Verificar que el J-Link aparece en "Universal Serial Bus controllers"

### 2.3 Instalar Drivers (si es necesario)

Si el dispositivo no es reconocido:
1. Descargar los drivers de SEGGER: https://www.segger.com/downloads/jlink/
2. Instalar **J-Link Software and Documentation Pack**

---

## Paso 3: Crear la Primera Aplicacion - LED Blink

### 3.1 Crear un Nuevo Proyecto

1. En VS Code, abrir el panel **nRF Connect**
2. Hacer clic en **Create a new application**
3. Seleccionar **Copy a sample**
4. Buscar y seleccionar: **zephyr/samples/basic/blinky**
5. Elegir ubicacion para el proyecto (ej: esta carpeta)
6. Nombrar el proyecto: `blinky_nrf9151`

### 3.2 Estructura del Proyecto

```
blinky_nrf9151/
├── CMakeLists.txt      # Configuracion de compilacion
├── prj.conf            # Configuracion del proyecto Zephyr
└── src/
    └── main.c          # Codigo fuente principal
```

### 3.3 Codigo del Blinky (main.c)

```c
/*
 * Ejemplo Blinky para nRF9151 DK
 * Hace parpadear el LED0 cada segundo
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* Intervalo de parpadeo en milisegundos */
#define SLEEP_TIME_MS   1000

/* Obtener el nodo del LED0 desde el Device Tree */
#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
    int ret;
    bool led_state = true;

    /* Verificar que el GPIO esta listo */
    if (!gpio_is_ready_dt(&led)) {
        printk("Error: GPIO del LED no esta listo\n");
        return -1;
    }

    /* Configurar el pin del LED como salida */
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error: No se pudo configurar el LED\n");
        return -1;
    }

    printk("Blinky iniciado en nRF9151 DK!\n");

    /* Bucle principal */
    while (1) {
        /* Alternar estado del LED */
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0) {
            printk("Error al cambiar estado del LED\n");
            return -1;
        }

        led_state = !led_state;
        printk("LED estado: %s\n", led_state ? "ON" : "OFF");

        /* Esperar */
        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}
```

### 3.4 Configuracion del Proyecto (prj.conf)

```conf
# Habilitar GPIO
CONFIG_GPIO=y

# Habilitar salida por consola serial
CONFIG_SERIAL=y
CONFIG_CONSOLE=y
CONFIG_UART_CONSOLE=y

# Configuracion de logs (opcional)
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=3
```

---

## Paso 4: Compilar y Flashear

### 4.1 Configurar el Build

1. En el panel **nRF Connect**, hacer clic en **Add Build Configuration**
2. Seleccionar el board: **nrf9151dk/nrf9151**
3. Hacer clic en **Build Configuration**

### 4.2 Compilar el Proyecto

1. Hacer clic en el icono **Build** (martillo) en la barra de acciones
2. Esperar a que la compilacion termine exitosamente
3. Verificar que no hay errores en la consola

### 4.3 Flashear al Dispositivo

1. Asegurar que el nRF9151 DK esta conectado via USB
2. Hacer clic en **Flash** en el panel nRF Connect
3. Esperar a que el proceso de flasheo termine

### 4.4 Comandos Alternativos (Terminal)

Si prefieres usar la terminal:

```bash
# Navegar al directorio del proyecto
cd blinky_nrf9151

# Compilar para nRF9151 DK
west build -b nrf9151dk/nrf9151

# Flashear
west flash
```

---

## Paso 5: Verificar el Funcionamiento

### 5.1 Observar el LED

- El **LED1** (led0) deberia comenzar a parpadear cada segundo
- El LED esta ubicado cerca del centro de la placa

### 5.2 Ver Mensajes de Debug

1. En VS Code, abrir una terminal serial:
   - Panel nRF Connect > **Connected Devices** > Click derecho > **Open Serial Terminal**
   - O usar: `Ctrl+Shift+P` > "nRF Terminal: Start Terminal"

2. Configuracion del puerto serial:
   - Baud rate: **115200**
   - Data bits: 8
   - Parity: None
   - Stop bits: 1

3. Deberias ver mensajes como:
   ```
   Blinky iniciado en nRF9151 DK!
   LED estado: ON
   LED estado: OFF
   LED estado: ON
   ...
   ```

---

## Mapeo de LEDs y Botones

### LEDs Disponibles

| Alias | LED | GPIO | Ubicacion en placa |
|-------|-----|------|-------------------|
| led0  | LED1 | P0.00 | Verde |
| led1  | LED2 | P0.01 | Verde |
| led2  | LED3 | P0.04 | Verde |
| led3  | LED4 | P0.05 | Verde |

### Botones Disponibles

| Alias | Boton | GPIO |
|-------|-------|------|
| sw0   | Button 1 | P0.08 |
| sw1   | Button 2 | P0.09 |
| sw2   | Button 3 | P0.18 |
| sw3   | Button 4 | P0.19 |

---

## Solucion de Problemas

### El dispositivo no es detectado

1. Verificar el cable USB (usar uno con datos, no solo carga)
2. Probar otro puerto USB
3. Reinstalar drivers de J-Link
4. Verificar que el switch de alimentacion esta en ON

### Error de compilacion "Board not found"

1. Verificar que el SDK esta correctamente instalado
2. En nRF Connect, ir a **Manage SDKs** y reinstalar si es necesario
3. Verificar el nombre del board: `nrf9151dk/nrf9151`

### El LED no parpadea

1. Verificar que el flasheo fue exitoso
2. Presionar el boton **RESET** en la placa
3. Revisar la consola serial por mensajes de error

### Error al flashear

1. Verificar conexion USB
2. Cerrar otras aplicaciones que usen el J-Link
3. Verificar que el switch **SW5** esta en posicion correcta para programacion interna

---

## Recursos Adicionales

- [Documentacion oficial nRF9151 DK](https://docs.nordicsemi.com/bundle/ncs-latest/page/zephyr/boards/nordic/nrf9151dk/doc/index.html)
- [Nordic Developer Academy - Curso Fundamentals](https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/)
- [Ejemplo Blinky en Zephyr](https://docs.nordicsemi.com/bundle/ncs-latest/page/zephyr/samples/basic/blinky/README.html)
- [nRF Connect for VS Code](https://nrfconnect.github.io/vscode-nrf-connect/index.html)
- [Documentacion Zephyr nRF9151 DK](https://docs.zephyrproject.org/latest/boards/nordic/nrf9151dk/doc/index.html)

---

## Siguiente Paso: Ejemplo con Boton

Una vez que el blinky funcione, puedes probar controlar el LED con un boton:

```c
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define LED0_NODE DT_ALIAS(led0)
#define SW0_NODE  DT_ALIAS(sw0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

int main(void)
{
    if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&button)) {
        return -1;
    }

    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&button, GPIO_INPUT);

    printk("Presiona el boton para encender el LED\n");

    while (1) {
        int val = gpio_pin_get_dt(&button);
        gpio_pin_set_dt(&led, val);
        k_msleep(10);
    }

    return 0;
}
```

---

*Guia creada para nRF9151 DK con nRF Connect SDK*
