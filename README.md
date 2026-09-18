# Deneyap ESP32-CAM UART Bridge

[![CI](https://github.com/berhankokum/deneyap-esp32cam-uart-bridge/actions/workflows/ci.yml/badge.svg)](https://github.com/berhankokum/deneyap-esp32cam-uart-bridge/actions/workflows/ci.yml)

A small ESP-IDF firmware project that turns a **Deneyap Kart 1A** into an active USB-to-UART bridge for programming and monitoring an **ESP32-CAM**.

This utility was created during development of the [Secure Visual Gateway](https://github.com/berhankokum/secure-visual-gateway) project when a dedicated USB-to-UART adapter was not available.

> This is a UART programming bridge, not a standalone hardware programmer. ESP32-CAM boot mode and reset are still controlled manually.

---

## What It Does

```text
PC
 |
 | USB
 v
Deneyap Kart 1A
 |
 | UART bridge firmware
 |
 +---- GPIO23 (TX) ------> ESP32-CAM U0R / RX
 |
 +---- GPIO22 (RX) <------ ESP32-CAM U0T / TX
```

The bridge forwards serial traffic bidirectionally so tools such as `esptool` can communicate with the ESP32-CAM through the Deneyap board.

---

## Hardware

- Deneyap Kart 1A
- ESP32-CAM
- USB cable

### UART Mapping

| Deneyap Kart 1A | GPIO | Direction | ESP32-CAM |
|---|---:|---|---|
| D0 | GPIO23 | Deneyap -> CAM | U0R / RX |
| D1 | GPIO22 | CAM -> Deneyap | U0T / TX |
| 5V | - | Power | 5V |
| GND | - | Ground | GND |

---

## Wiring

```text
Deneyap Kart 1A                  ESP32-CAM

5V ---------------------------> 5V
GND --------------------------> GND

D0 / GPIO23 ------------------> U0R / RX
D1 / GPIO22 <------------------ U0T / TX
```

To enter the ESP32-CAM ROM download mode:

```text
ESP32-CAM IO0 -----------------> GND
```

Then reset or power-cycle the ESP32-CAM.

Do **not** connect the Deneyap `EN` pin to GND while the active bridge firmware is running.

---

## Baud Rate

The bridge uses:

```text
115200 baud
```

---

## How It Works

```text
PC USB/UART
    |
    v
Deneyap UART0
    |
    | bidirectional forwarding
    v
Deneyap UART1
    |
    v
ESP32-CAM UART0
```

Target-side pins:

```text
TX: GPIO23
RX: GPIO22
```

The bridge waits for incoming serial data and then drains available bytes in batches, reducing unnecessary forwarding latency.

---

## Repository Structure

```text
deneyap-esp32cam-uart-bridge/
|
+-- main/
|   +-- CMakeLists.txt
|   +-- deneyap_uart_bridge.c
|
+-- .github/
|   +-- workflows/
|       +-- ci.yml
|
+-- CMakeLists.txt
+-- sdkconfig
+-- .gitignore
+-- LICENSE
+-- README.md
```

---

## Requirements

- ESP-IDF 6.1
- Python
- esptool

---

## Build

```powershell
idf.py set-target esp32
idf.py build
```

---

## Flash the Bridge

Connect the Deneyap board to the PC:

```powershell
idf.py -p COM9 flash
```

---

## Program an ESP32-CAM Through the Bridge

1. Flash the bridge firmware to the Deneyap board.
2. Disconnect USB power.
3. Connect:

```text
Deneyap 5V       -> ESP32-CAM 5V
Deneyap GND      -> ESP32-CAM GND
Deneyap GPIO23   -> ESP32-CAM U0R / RX
Deneyap GPIO22   <- ESP32-CAM U0T / TX
ESP32-CAM IO0    -> GND
```

4. Reconnect USB and reset or power-cycle the ESP32-CAM.
5. From the target firmware's `build` directory, run:

```powershell
python -m esptool --chip esp32 -p COM9 -b 115200 --before no-reset --after no-reset write-flash "@flash_args"
```

The `no-reset` options are used because this bridge does not automatically control the target `EN` and `IO0` pins.

6. After flashing, disconnect power, remove `IO0 -> GND`, reconnect power, and boot the ESP32-CAM normally.

---

## Serial Monitoring

For simple target log monitoring:

```powershell
python -m serial.tools.miniterm COM9 115200
```

Using `idf.py monitor` with the bridge project's ELF while viewing target firmware output may show misleading ELF/checksum messages because the connected serial output belongs to the ESP32-CAM, not the bridge application.

---

## Limitations

- Manual ESP32-CAM boot-mode selection
- Manual target reset
- No automatic DTR/RTS bootloader control
- Fixed GPIO23/GPIO22 target UART pins
- Fixed 115200 baud configuration
- Intended as a development utility, not a production programmer

---

## Possible Improvements

- automatic `EN` / `IO0` control
- configurable baud rate
- configurable UART pins
- serial activity counters
- status LEDs
- command mode for reset/boot control

---

## Continuous Integration

GitHub Actions builds the project using ESP-IDF 6.1 for the ESP32 target on every push and pull request to `main`.

---

## License

MIT. See [`LICENSE`](LICENSE).
