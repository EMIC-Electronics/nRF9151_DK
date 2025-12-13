# Resumen de Sesion - nRF9151 DK

**Fecha:** 12 de Diciembre 2025
**Repositorio:** https://github.com/EMIC-Electronics/nRF9151_DK

---

## Objetivos Cumplidos

### 1. Configuracion del Entorno de Desarrollo

- [x] VS Code con nRF Connect Extension Pack
- [x] nRF Connect SDK v3.2.0 instalado en `C:\ncs`
- [x] SEGGER J-Link V7.94i
- [x] Repositorio GitHub creado y configurado
- [x] Colaborador agregado: lugaro95

### 2. Hardware Verificado

| Componente | Detalle |
|------------|---------|
| Placa | nRF9151 DK (PCA10171) |
| Puerto Serie Modem | COM10 |
| Puerto Serie App | COM11 (115200 baud) |
| Almacenamiento | Volumen H: (J-Link Mass Storage) |
| SIM incluida | Onomondo |

### 3. Guias Creadas

| Archivo | Descripcion | Estado |
|---------|-------------|--------|
| `1-GUIA_INICIO_nRF9151_DK.md` | Instalacion SDK, primer proyecto (Blinky) | ✅ Completo |
| `2-GUIA_TCP_CLIENT.md` | Cliente TCP con LTE-M | ✅ Probado exitosamente |
| `3-GUIA_TCP_NBIOT.md` | Cliente TCP con NB-IoT | ⏳ Pendiente (sin cobertura) |

### 4. Proyectos Creados

```
nRF9151_DK/
├── blinky/              # LED parpadeante (ejemplo basico)
├── tcp_client/          # Cliente TCP via LTE-M
└── tcp_client_nbiot/    # Cliente TCP via NB-IoT
```

### 5. Resultados de Pruebas

#### LTE-M (tcp_client) - EXITOSO ✅

```
Conexion LTE: Roaming (Onomondo SIM)
Servidor: 72.60.48.208:5555
TX: "hola mundo"
RX: "HOLA MUNDO"
Estado: Funcionando correctamente
```

#### NB-IoT (tcp_client_nbiot) - PENDIENTE ⏳

- Probado con 3 SIMs: Onomondo, Conexa, Personal/Telecom
- Resultado: EMM cause 15/19 (sin cobertura NB-IoT en Buenos Aires)
- Accion: Probar en ubicacion con cobertura NB-IoT

---

## Tecnologias Soportadas por nRF9151

| Tecnologia | Soporte | Velocidad | Uso |
|------------|---------|-----------|-----|
| LTE (4G tradicional) | ❌ No | - | - |
| LTE-M (Cat-M1) | ✅ Si | ~1 Mbps | Trackers, wearables |
| NB-IoT | ✅ Si | ~250 kbps | Sensores, medidores |
| NB-NTN (Satelite) | ✅ Si* | 20-40 kbps | IoT remoto/global |

*Firmware comercial NTN programado para principios 2026

---

## Proximos Pasos: Conectividad Satelital con Sateliot

### Estado del Proyecto Sateliot

| Item | Estado |
|------|--------|
| Contrato con Sateliot | ✅ Firmado |
| Reunion soporte tecnico | ⏳ Pendiente |
| Firmware NTN | ⏳ Solicitar a Nordic |
| Hardware NTN | ⚠️ Evaluar necesidad de nRF9151 SMA DK |

### Informacion Tecnica Sateliot

- **Tecnologia:** NB-IoT NTN (3GPP Release 17)
- **Satelites:** Constelacion LEO (600-800 km de altitud)
- **Velocidad:** 20-40 kbps
- **Ventaja:** Cobertura global (75% de la Tierra sin cobertura terrestre)

### Acciones Pendientes

#### 1. Reunion con Soporte Tecnico Sateliot
- [ ] Confirmar fecha de reunion
- [ ] Preparar preguntas tecnicas:
  - Bandas de frecuencia utilizadas
  - Requisitos de antena
  - Proceso de activacion de SIM/servicio
  - Cobertura satelital sobre Argentina
  - Tiempos de paso de satelites (ventanas de conectividad)

#### 2. Solicitar Firmware NTN a Nordic
- [ ] Contactar a Nordic Semiconductor
- [ ] Solicitar firmware pre-comercial con soporte NB-NTN
- [ ] Verificar compatibilidad con nRF9151 DK actual

#### 3. Evaluar Hardware
- [ ] Determinar si el nRF9151 DK actual es suficiente para pruebas NTN
- [ ] Considerar adquirir **nRF9151 SMA DK** si se requiere:
  - Conectores SMA para antenas externas
  - Antenas Taoglas optimizadas para NTN
  - Mejor caracterizacion RF

#### 4. Preparar Entorno de Pruebas
- [ ] Obtener antena compatible con banda satelital NTN
- [ ] Configurar ubicacion de prueba (cielo despejado, sin obstrucciones)
- [ ] Preparar scripts de prueba similares a tcp_client

#### 5. Crear Guia NTN
- [ ] Documentar proceso de configuracion NTN
- [ ] Crear proyecto `tcp_client_ntn/` o `sateliot_client/`
- [ ] Agregar configuracion especifica:
  ```conf
  # Configuracion NB-NTN (a confirmar con Nordic/Sateliot)
  CONFIG_LTE_NETWORK_MODE_NBIOT_GPS=y
  CONFIG_NRF_MODEM_LIB_SHMEM_TX_SIZE=8192
  # Parametros NTN adicionales segun documentacion
  ```

### Recursos Sateliot

- **Web:** https://sateliot.space/
- **3GPP Rel-17:** https://sateliot.space/es/3gpp-rel-17/
- **Tecnologia NTN:** https://www.3gpp.org/technologies/ntn-overview

### Recursos Nordic NTN

- [Nordic + Sateliot LEO Test](https://www.nordicsemi.com/Nordic-news/2025/10/Successful-transmission-using-Nordics-nRF9151-module-and-Sateliots-LEO-satellite)
- [What is NTN - Nordic](https://www.nordicsemi.com/Products/Wireless/Low-power-cellular-IoT/What-is-NTN)
- [nRF9151 NTN at MWC 2025](https://www.nordicsemi.com/Nordic-news/2025/02/Nordic-Semiconductor-showcases-nRF9151-Non-Terrestrial-Network-capabilities-at-MWC-2025)

### Preguntas para la Reunion con Sateliot

1. **Activacion:**
   - ¿Como se activa el servicio en nuestro dispositivo?
   - ¿Se requiere una SIM especial o eSIM?

2. **Firmware:**
   - ¿Que version de firmware Nordic se recomienda?
   - ¿Hay SDK o ejemplos de codigo disponibles?

3. **Antena:**
   - ¿Que especificaciones debe tener la antena?
   - ¿La antena del nRF9151 DK es compatible?

4. **Cobertura:**
   - ¿Cual es la cobertura actual sobre Argentina?
   - ¿Horarios de paso de satelites?
   - ¿Latencia esperada?

5. **Integracion:**
   - ¿Como se enrutan los datos (nRF Cloud, servidor propio)?
   - ¿Protocolo recomendado (TCP, UDP, CoAP)?

6. **Costos:**
   - ¿Modelo de facturacion (por mensaje, por byte)?
   - ¿Limites de uso en plan contratado?

---

## Comandos Utiles

### Compilar y Flashear

```bash
# Configurar entorno (Git Bash)
export ZEPHYR_BASE="/c/ncs/v3.2.0/zephyr"
export PATH="/c/ncs/toolchains/b620d12352/opt/bin/Scripts:$PATH"

# Compilar
west build -b nrf9151dk/nrf9151/ns --pristine

# Flashear
west flash --runner jlink
```

### Monitor Serie

- Puerto: COM11
- Baud rate: 115200

---

## Contactos

- **Repositorio:** https://github.com/EMIC-Electronics/nRF9151_DK
- **Colaboradores:** mhunkeler, lugaro95

---

*Documento generado el 12 de Diciembre 2025*
