# Proyecto de Comunicación Multi-nodo con LoRa en Red Mesh

Este repositorio contiene el código fuente, documentación técnica y archivos auxiliares del sistema de comunicación multi-nodo diseñado para microcontroladores ESP32 con modulación LoRa y topología de red mesh parcial. El proyecto fue desarrollado como parte de un trabajo de titulación, priorizando una arquitectura descentralizada, tolerante a fallos y adaptable.

## 📁 Estructura del Repositorio

```
LoRaMeshProject/
├── src/                       # Código fuente principal
│   └── LoRaMesh/             # Lógica modular del sistema
│       ├── LoRaMesh.ino
│       ├── communication_manager.h
│       ├── config.h
│       ├── lora_manager.h
│       ├── message_receiver.h
│       ├── message_scheduler.h
│       ├── oled_manager.h
│       ├── packet_manager.h
│       └── routing_manager.h
├── docs/                     # Archivos auxiliares
│   ├── diagrama_gpio.png
│   ├── topologia_mesh.png
│   ├── case_nodo.STL
│   ├── stand_nodo.STL
│   └── CP210x_Windows_Drivers.zip
├── README.md                 # Este archivo
├── .gitignore                # Exclusiones para Git
```

## 🔧 Requisitos

- Placas **Heltec Wireless Stick V3** (ESP32-S3 + SX1262)
- Arduino IDE 2.3.2 o superior
- Driver CP210x USB to UART para cargar el firmware
- Librerías:
  - `LoRaWan_APP.h`
  - `HT_SSD1306Wire.h`
  - `Wire.h`
  - `esp_system.h`
  - `string.h`
  - `Arduino.h`

## ⚙️ Descripción del Sistema

El sistema opera desde la capa física hasta la capa de red del modelo OSI, permitiendo la transmisión y reenvío de mensajes entre nodos mediante los siguientes tipos de paquetes:

- `DATA`: Paquete de datos con TTL, payload y control de ruta.
- `ACK`: Confirmación hop-by-hop de la entrega de paquetes.
- `HELLO`: Descubrimiento de vecinos.
- `ALT`: Notificación de rutas fallidas o congestionadas.

### Mecanismos implementados

- Enrutamiento basado en vecinos y métricas locales.
- Confirmación de entrega por saltos (hop-by-hop).
- Detección de duplicados y ventanas de escucha tipo LBT.
- Reconvergencia automática ante fallos sin intervención externa.
- Planificador de colas por tipo de paquete (prioridad).

## 🧠 Arquitectura

El sistema está compuesto por:

- **Hardware**: ESP32-S3 (Wireless Stick V3), OLED integrada, chip SX1262.
- **Topología**: Mesh parcial con 5 nodos (A, B, C, D, E), con rutas alternativas.
- **Software**: Modularizado en C++ usando Arduino IDE, sin LoRaWAN, basado en funciones propias de comunicación multinodo.

## 🧪 Uso

1. Clona el repositorio:
2. Abre el archivo `LoRaMesh.ino` en Arduino IDE.
3. Compila y sube a cada nodo ESP32.
4. Observa la comunicación entre nodos en el monitor serial o en la pantalla OLED.

## 📎 Archivos Adicionales

- Diagramas de conexión GPIO (`docs/diagrama_gpio.jpg`)
- Diagrama de topología mesh (`docs/topologia_mesh.png`)
- Driver USB CP210x (`docs/driver_CP210x.zip`)
- Archivos STL para impresión 3D:
  - Case del nodo (`docs/case_nodo.STL`)
  - Stand o soporte del nodo (`docs/stand_nodo.STL`)

## 📚 Créditos

Desarrollado por Danilo Arregui como parte del trabajo de titulación de Ingeniería en Tecnologías de la Información - PUCE 2025.
