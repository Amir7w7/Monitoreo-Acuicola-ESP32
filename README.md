# Monitoreo Acuícola con ESP32

Código del sistema de monitoreo acuícola desarrollado con dos nodos ESP32.

## Arquitectura

```text
Temperatura / pH / turbidez
          ↓
     ESP32 boya
          ↓ ESP-NOW
    ESP32 maestro
          ↓ WiFi
       Internet
          ↓
   Webhook Make.com
```

- **Comunitarias**: adquisición de temperatura, pH y turbidez y transmisión mediante ESP-NOW.
- **ComunitariasMaestro**: recepción ESP-NOW, visualización OLED, semáforo de estado y envío HTTP POST/JSON a Make.com.

La integración de Make.com con Telegram se configura en la plataforma Make.com y no está contenida en estos archivos fuente.

## Hardware y software

- ESP32
- DS18B20 para temperatura
- Entrada analógica para pH
- Entrada analógica para turbidez
- OLED SSD1306 en el nodo maestro
- PlatformIO + framework Arduino
- ESP-NOW, WiFi y HTTP/JSON

## Configuración antes de compilar

Por seguridad, esta versión pública no contiene las credenciales ni identificadores reales del sistema. Deben sustituirse:

- `TU_RED_WIFI`
- `TU_PASSWORD_WIFI`
- `https://hook.REGION.make.com/TU_WEBHOOK`
- La MAC `00:00:00:00:00:00` del nodo maestro en `Comunitarias/src/main.cpp`

## Nota

El repositorio conserva la lógica de la versión entregada del proyecto y se publica como respaldo técnico de la arquitectura implementada. Los archivos generados por PlatformIO (`.pio`) no se incluyen.
