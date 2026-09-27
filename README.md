# Aqua Mate - Real-Time Water Temperature Monitoring System

An IoT-based real-time water temperature monitoring dashboard built with **ESP32**, **DS18B20 Waterproof Temperature Sensor**, and a **2.4" ILI9341 SPI TFT Display**. This module is a core hardware component of the **Aqua Mate** smart aquaculture ecosystem.

---

## 📌 Project Overview

This project provides high-precision water temperature measurement with real-time visual telemetry. It incorporates signal filtering, automatic error detection, dynamic color-coded UI cards, and SPI bus lock protection to ensure continuous, rock-solid operation.

### Key Features
- **High-Precision Sensing:** Uses DS18B20 waterproof sensor with Exponential Moving Average (EMA) noise reduction filter.
- **Dual-Card Visual Dashboard:** Clean $320 \times 240$ UI presenting concurrent Celsius (°C) and Fahrenheit (°F) telemetry without flickering.
- **Dynamic Color Coding:**
  - 🧊 **Cold Water (< 20°C):** Cyan
  - 🟢 **Optimal Water (20°C - 32°C):** Green
  - ⚠️ **High Temperature Warning (> 32°C):** Red
- **Fault Tolerance:** Live detection for disconnected sensors or out-of-range readings with on-screen error diagnostics.
- **Hardware SPI Optimization:** Dedicated GPIO mapping and forced Touch Chip-Select (CS) locking to avoid SPI bus collisions.

---

## 🛠️ Hardware Requirements

| Component | Specification / Quantity |
| :--- | :--- |
| **Microcontroller** | ESP32 Development Board (30-Pin) |
| **Display** | ILI9341 2.4" SPI TFT LCD (240x320) |
| **Sensor** | DS18B20 Waterproof Temperature Sensor |
| **Pull-Up Resistor** | $1\times 4.7\text{k}\Omega$ OR $2\times 10\text{k}\Omega$ (Parallel = $5\text{k}\Omega$) |
| **Power & Wiring** | Breadboard & Male-to-Female / Male-to-Male Jumpers |

---

## 🔌 Connection Mapping & Pinout Tables

### 1. DS18B20 Temperature Sensor Wiring

| Sensor Wire Color | ESP32 Pin Label | Pin Function / Connection Note |
| :--- | :--- | :--- |
| **Red (VCC)** | **VIN** (5V) | Power input for stable sensor operation |
| **Black (GND)** | **GND** | Common Ground |
| **Yellow (Data)** | **D27** (GPIO 27) | Dedicated OneWire Data Pin |

> **⚠️ Resistor Setup Note:**
> Connect a **$4.7\text{k}\Omega$ pull-up resistor** (or two $10\text{k}\Omega$ resistors in parallel) between the **Yellow Data Wire (D27)** and **3.3V**. 
> *Do not connect the pull-up resistor to 5V/VIN to protect ESP32 GPIO voltage limits.*

---

### 2. ILI9341 TFT Display Module Wiring

| Display Pin (TFT Label) | ESP32 Pin Label | Function / Description |
| :--- | :--- | :--- |
| **VCC** | **VIN** (5V) | Display Power Input |
| **GND** | **GND** | Common Ground |
| **CS** | **D5** (GPIO 5) | Display Chip Select |
| **RESET / RST** | **D16** (GPIO 16) | Hardware Reset Pin |
| **DC / RS** | **D2** (GPIO 2) | Data / Command Control |
| **SDI (MOSI)** | **D23** (GPIO 23) | Hardware SPI Data Line |
| **SCK** | **D18** (GPIO 18) | Hardware SPI Clock Line |
| **LED / BLK** | **3.3V** | Display Backlight Power |
| **SDO (MISO)** | **D19** (GPIO 19) | Hardware SPI Output Line |

---

### 3. Touch Controller Pins (XPT2046 - Physical Header Order)

Touch pins follow the exact physical sequence printed on the display board header. It shares the hardware SPI bus with the ILI9341 display.

| Touch Pin (TFT Label) | ESP32 Pin Label | Function / Description |
| :--- | :--- | :--- |
| **T_CLK** | **D18** (GPIO 18) | Hardware SPI Clock (Shared with TFT SCK) |
| **T_CS** | **D13** (GPIO 13) | Touch Chip Select Pin |
| **T_DIN** | **D23** (GPIO 23) | Hardware SPI Data Input (Shared with TFT SDI/MOSI) |
| **T_DO** | **D19** (GPIO 19) | Hardware SPI Data Output (Shared with TFT SDO/MISO) |
| **T_IRQ** | **VP** (GPIO 36) *OR* Unconnected | Touch Interrupt Pin (Optional / Polling mode used) |

---

## 💻 Software & Dependencies

Ensure the following libraries are installed in Arduino IDE (`Tools -> Manage Libraries`):

1. **`OneWire`** by Paul Stoffregen
2. **`DallasTemperature`** by Miles Burton
3. **`Adafruit GFX Library`** by Adafruit
4. **`Adafruit ILI9341`** by Adafruit
5. **`XPT2046_Touchscreen`** by Paul
---

## 🚀 How to Run

1. Wire the components according to the **Connection Mapping** tables above.
2. Open Arduino IDE and select **ESP32 Dev Module** as the target board.
3. Paste the firmware code into the IDE.
4. Connect your ESP32 via USB and click **Upload**.
5. Open Serial Monitor at **115200 baud** to view real-time telemetry logs alongside the TFT display.
