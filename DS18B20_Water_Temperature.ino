#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

// -------------------------------------------------------------------
// Hardware Pin Configuration
// -------------------------------------------------------------------
#define ONE_WIRE_BUS 27

#define TFT_CS       5
#define TFT_DC       2
#define TFT_RST      16
#define TOUCH_CS     13

// Touch Screen Calibration Constants (Rotation 1)
#define TS_MINX 200
#define TS_MAXX 3800
#define TS_MINY 200
#define TS_MAXY 3800

// -------------------------------------------------------------------
// Object Instantiations
// -------------------------------------------------------------------
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS);

// -------------------------------------------------------------------
// Global Variables & Filtering
// -------------------------------------------------------------------
const unsigned long SENSOR_READ_INTERVAL = 1200;
const unsigned long TOUCH_DEBOUNCE_DELAY = 300;

unsigned long lastSensorReadTime = 0;
unsigned long lastTouchTime = 0;

const float EMA_ALPHA = 0.3;
float filteredTempC = 0.0;
bool isFirstReading = true;

enum DisplayMode {
  MODE_DASHBOARD,
  MODE_COLOR_KEY
};

enum SensorStatus {
  STATUS_OK,
  STATUS_DISCONNECTED,
  STATUS_OUT_OF_BOUNDS
};

DisplayMode currentMode = MODE_DASHBOARD;
SensorStatus lastStatus = STATUS_OK;
float currentRawC = 0.0;

// -------------------------------------------------------------------
// Function Prototypes
// -------------------------------------------------------------------
void initDashboardUI();
void renderColorKeyModal();
void updateTFTDisplay(float tempC, float tempF, SensorStatus status);
void checkTouchInputs();
uint16_t getTemperatureColor(float tempC);

// -------------------------------------------------------------------
// Setup Function
// -------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("\n[INFO] Aqua Mate Monitoring System Initializing..."));

  // Initialize Display & Touch
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  touch.begin();
  touch.setRotation(1);

  // Initialize Sensor
  sensors.begin();
  sensors.setResolution(12);
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();

  // Draw Initial Screen
  initDashboardUI();
  Serial.println(F("[INFO] System Ready. Dashboard loaded."));
}

// -------------------------------------------------------------------
// Main Loop
// -------------------------------------------------------------------
void loop() {
  unsigned long currentMillis = millis();

  // 1. Process Touch Inputs
  checkTouchInputs();

  // 2. Periodic Sensor Reading & Filtering
  if (currentMillis - lastSensorReadTime >= SENSOR_READ_INTERVAL) {
    lastSensorReadTime = currentMillis;

    currentRawC = sensors.getTempCByIndex(0);
    sensors.requestTemperatures(); // Request next conversion

    SensorStatus currentStatus = STATUS_OK;

    if (currentRawC == DEVICE_DISCONNECTED_C) {
      currentStatus = STATUS_DISCONNECTED;
    } else if (currentRawC < -55.0 || currentRawC > 125.0 || currentRawC == 85.0) {
      currentStatus = STATUS_OUT_OF_BOUNDS;
    }

    float tempF = 0.0;
    if (currentStatus == STATUS_OK) {
      if (isFirstReading) {
        filteredTempC = currentRawC;
        isFirstReading = false;
      } else {
        filteredTempC = (EMA_ALPHA * currentRawC) + ((1.0 - EMA_ALPHA) * filteredTempC);
      }
      tempF = (filteredTempC * 9.0 / 5.0) + 32.0;
    }

    lastStatus = currentStatus;

    // Serial Monitor Output (Restored!)
    unsigned long sec = currentMillis / 1000;
    Serial.print("[TIME: ");
    Serial.print(sec);
    Serial.print("s] | STATUS: ");
    if (currentStatus == STATUS_OK) {
      Serial.print("OK | Raw: ");
      Serial.print(currentRawC, 2);
      Serial.print(" *C | Filtered: ");
      Serial.print(filteredTempC, 2);
      Serial.print(" *C / ");
      Serial.print(tempF, 2);
      Serial.println(" *F");
    } else if (currentStatus == STATUS_DISCONNECTED) {
      Serial.println("DISCONNECTED");
    } else {
      Serial.println("OUT_OF_BOUNDS");
    }

    // Update display only when in Dashboard mode
    if (currentMode == MODE_DASHBOARD) {
      updateTFTDisplay(filteredTempC, tempF, currentStatus);
    }
  }
}

// -------------------------------------------------------------------
// Touch Input Handling (Fixed Mapping & Inversion)
// -------------------------------------------------------------------
void checkTouchInputs() {
  if (touch.touched()) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastTouchTime < TOUCH_DEBOUNCE_DELAY) return;
    lastTouchTime = currentMillis;

    TS_Point p = touch.getPoint();

    // Correct mapping for Rotation 1 (Landscape 320x240)
    // Note: X and Y are mapped to match display orientation
    int touchX = map(p.y, TS_MINY, TS_MAXY, 0, 320);
    int touchY = map(p.x, TS_MINX, TS_MAXX, 0, 240);

    // Constrain coordinates within screen limits
    touchX = constrain(touchX, 0, 320);
    touchY = constrain(touchY, 0, 240);

    // Debug print for touch
    Serial.print("[TOUCH DETECTED] Raw X: ");
    Serial.print(p.x);
    Serial.print(" | Raw Y: ");
    Serial.print(p.y);
    Serial.print(" --> Mapped X: ");
    Serial.print(touchX);
    Serial.print(" | Mapped Y: ");
    Serial.println(touchY);

    // If viewing Color Key, tap anywhere to return to Dashboard
    if (currentMode == MODE_COLOR_KEY) {
      currentMode = MODE_DASHBOARD;
      initDashboardUI();
      float tempF = (filteredTempC * 9.0 / 5.0) + 32.0;
      updateTFTDisplay(filteredTempC, tempF, lastStatus);
      Serial.println(F("[INFO] Returned to Dashboard Mode."));
      return;
    }

    // Touch Detection for Dashboard Mode
    // Bottom Button Area: COLOR KEY [x: 10..310, y: 194..232]
    if (currentMode == MODE_DASHBOARD) {
      if (touchY >= 194 && touchY <= 235 && touchX >= 10 && touchX <= 310) {
        currentMode = MODE_COLOR_KEY;
        renderColorKeyModal();
        Serial.println(F("[INFO] Opened Color Key Modal."));
      }
    }
  }
}

// -------------------------------------------------------------------
// Dynamic Color Determination
// -------------------------------------------------------------------
uint16_t getTemperatureColor(float tempC) {
  if (tempC < 20.0) {
    return ILI9341_CYAN;     // Cold Water
  } else if (tempC <= 32.0) {
    return ILI9341_GREEN;    // Optimal Temperature
  } else {
    return ILI9341_RED;      // High Temperature Warning
  }
}

// -------------------------------------------------------------------
// UI Rendering Functions
// -------------------------------------------------------------------

void initDashboardUI() {
  tft.fillScreen(ILI9341_BLACK);

  // Header Banner
  tft.fillRect(0, 0, 320, 36, ILI9341_NAVY);
  tft.drawFastHLine(0, 36, 320, ILI9341_CYAN);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(50, 10);
  tft.print(F("AQUA MATE MONITOR"));

  // Left Card: Celsius Panel
  tft.drawRoundRect(10, 44, 145, 142, 8, ILI9341_CYAN);
  tft.drawRoundRect(11, 45, 143, 140, 7, ILI9341_DARKCYAN);
  tft.setTextColor(ILI9341_CYAN);
  tft.setTextSize(1);
  tft.setCursor(32, 56);
  tft.print(F("CELSIUS (C)"));
  tft.drawFastHLine(20, 70, 125, ILI9341_DARKGREY);

  // Right Card: Fahrenheit Panel
  tft.drawRoundRect(165, 44, 145, 142, 8, ILI9341_BLUE);
  tft.drawRoundRect(166, 45, 143, 140, 7, ILI9341_NAVY);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(182, 56);
  tft.print(F("FAHRENHEIT (F)"));
  tft.drawFastHLine(175, 70, 125, ILI9341_DARKGREY);

  // Touch Interactive Button at Bottom: COLOR KEY
  tft.fillRoundRect(10, 194, 300, 38, 6, ILI9341_DARKCYAN);
  tft.drawRoundRect(10, 194, 300, 38, 6, ILI9341_CYAN);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(45, 205);
  tft.print(F("[ TOUCH FOR COLOR KEY ]"));
}

void updateTFTDisplay(float tempC, float tempF, SensorStatus status) {
  // Clear value bounding areas to eliminate flickering
  tft.fillRect(15, 82, 135, 95, ILI9341_BLACK);
  tft.fillRect(170, 82, 135, 95, ILI9341_BLACK);

  if (status != STATUS_OK) {
    // Error Handling Render
    tft.fillRoundRect(10, 194, 300, 38, 6, ILI9341_MAROON);
    tft.drawRoundRect(10, 194, 300, 38, 6, ILI9341_RED);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.setCursor(35, 208);

    if (status == STATUS_DISCONNECTED) {
      tft.print(F("ERROR: SENSOR DISCONNECTED (D27)"));
    } else {
      tft.print(F("ERROR: SENSOR OUT OF BOUNDS"));
    }

    tft.setTextColor(ILI9341_RED);
    tft.setTextSize(2);
    tft.setCursor(32, 115);
    tft.print(F("OFFLINE"));
    tft.setCursor(187, 115);
    tft.print(F("OFFLINE"));
    return;
  }

  // Determine Dynamic Color for BOTH C and F
  uint16_t dynamicColor = getTemperatureColor(tempC);

  // Render Celsius Readout
  tft.setTextColor(dynamicColor);
  tft.setTextSize(3);
  tft.setCursor(20, 100);
  tft.print(tempC, 2);
  tft.setTextSize(2);
  tft.print(F(" C"));

  // Render Fahrenheit Readout (Dynamic Color Applied!)
  tft.setTextColor(dynamicColor);
  tft.setTextSize(3);
  tft.setCursor(175, 100);
  tft.print(tempF, 1);
  tft.setTextSize(2);
  tft.print(F(" F"));

  // Status Range Indicator inside Cards
  tft.setTextSize(1);
  tft.setCursor(35, 155);
  if (dynamicColor == ILI9341_CYAN) {
    tft.print(F("[ STATUS: COLD ]"));
  } else if (dynamicColor == ILI9341_GREEN) {
    tft.print(F("[ STATUS: IDEAL ]"));
  } else {
    tft.print(F("[ STATUS: HIGH! ]"));
  }

  tft.setCursor(190, 155);
  if (dynamicColor == ILI9341_CYAN) {
    tft.print(F("[ STATUS: COLD ]"));
  } else if (dynamicColor == ILI9341_GREEN) {
    tft.print(F("[ STATUS: IDEAL ]"));
  } else {
    tft.print(F("[ STATUS: HIGH! ]"));
  }
}

void renderColorKeyModal() {
  tft.fillScreen(ILI9341_BLACK);

  // Header Banner
  tft.fillRect(0, 0, 320, 36, ILI9341_NAVY);
  tft.drawFastHLine(0, 36, 320, ILI9341_CYAN);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(40, 10);
  tft.print(F("TEMPERATURE COLOR KEY"));

  // Cyan Legend Box
  tft.fillRoundRect(15, 48, 290, 42, 6, ILI9341_DARKCYAN);
  tft.drawRoundRect(15, 48, 290, 42, 6, ILI9341_CYAN);
  tft.setTextColor(ILI9341_CYAN);
  tft.setTextSize(1);
  tft.setCursor(25, 63);
  tft.print(F("CYAN : < 20 C / < 68 F"));
  tft.setCursor(225, 63);
  tft.print(F("[ COLD ]"));

  // Green Legend Box
  tft.fillRoundRect(15, 98, 290, 42, 6, ILI9341_DARKGREEN);
  tft.drawRoundRect(15, 98, 290, 42, 6, ILI9341_GREEN);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(1);
  tft.setCursor(25, 113);
  tft.print(F("GREEN: 20-32 C / 68-90 F"));
  tft.setCursor(215, 113);
  tft.print(F("[ OPTIMAL ]"));

  // Red Legend Box
  tft.fillRoundRect(15, 148, 290, 42, 6, ILI9341_MAROON);
  tft.drawRoundRect(15, 148, 290, 42, 6, ILI9341_RED);
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(1);
  tft.setCursor(25, 163);
  tft.print(F("RED  : > 32 C / > 90 F"));
  tft.setCursor(220, 163);
  tft.print(F("[ HIGH ]"));

  // Return Prompt at Bottom
  tft.fillRoundRect(15, 198, 290, 32, 6, ILI9341_DARKGREY);
  tft.drawRoundRect(15, 198, 290, 32, 6, ILI9341_WHITE);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(50, 208);
  tft.print(F(">>> TOUCH ANYWHERE TO RETURN <<<"));
}