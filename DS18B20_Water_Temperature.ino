#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>

#define ONE_WIRE_BUS 27

#define TFT_CS       5
#define TFT_DC       2
#define TFT_RST      16
#define TOUCH_CS     13

const unsigned long SENSOR_READ_INTERVAL = 1500;
unsigned long lastSensorReadTime = 0;

const float EMA_ALPHA = 0.3;
float filteredTempC = 0.0;
bool isFirstReading = true;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

enum SensorStatus {
  STATUS_OK,
  STATUS_DISCONNECTED,
  STATUS_OUT_OF_BOUNDS
};

void initDashboard();
void updateTFTDisplay(float tempC, float tempF, SensorStatus status);
void printSerialTelemetry(float rawC, float filteredC, float filteredF, SensorStatus status);

void setup() {
  Serial.begin(115200);

  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  sensors.begin();
  sensors.setResolution(12);
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();

  initDashboard();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastSensorReadTime >= SENSOR_READ_INTERVAL) {
    lastSensorReadTime = currentMillis;

    float rawTempC = sensors.getTempCByIndex(0);
    sensors.requestTemperatures();

    SensorStatus currentStatus = STATUS_OK;

    if (rawTempC == DEVICE_DISCONNECTED_C) {
      currentStatus = STATUS_DISCONNECTED;
    } else if (rawTempC < -55.0 || rawTempC > 125.0 || rawTempC == 85.0) {
      currentStatus = STATUS_OUT_OF_BOUNDS;
    }

    float tempF = 0.0;
    if (currentStatus == STATUS_OK) {
      if (isFirstReading) {
        filteredTempC = rawTempC;
        isFirstReading = false;
      } else {
        filteredTempC = (EMA_ALPHA * rawTempC) + ((1.0 - EMA_ALPHA) * filteredTempC);
      }
      tempF = (filteredTempC * 9.0 / 5.0) + 32.0;
    }

    printSerialTelemetry(rawTempC, filteredTempC, tempF, currentStatus);
    updateTFTDisplay(filteredTempC, tempF, currentStatus);
  }
}

void printSerialTelemetry(float rawC, float filteredC, float filteredF, SensorStatus status) {
  unsigned long uptimeSec = millis() / 1000;
  Serial.print(F("[TIME: "));
  if (uptimeSec < 10) Serial.print(F("0"));
  Serial.print(uptimeSec);
  Serial.print(F("s] | STATUS: "));

  if (status == STATUS_OK) {
    Serial.print(F("OK | Raw: "));
    Serial.print(rawC, 2);
    Serial.print(F(" *C | Filtered: "));
    Serial.print(filteredC, 2);
    Serial.print(F(" *C / "));
    Serial.print(filteredF, 2);
    Serial.println(F(" *F"));
  } else if (status == STATUS_DISCONNECTED) {
    Serial.println(F("ERROR - DS18B20 Disconnected!"));
  } else {
    Serial.println(F("WARNING - Invalid Sensor Reading!"));
  }
}

void initDashboard() {
  tft.fillScreen(ILI9341_BLACK);

  // Header Banner
  tft.fillRect(0, 0, 320, 38, ILI9341_NAVY);
  tft.drawFastHLine(0, 38, 320, ILI9341_CYAN);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(60, 11);
  tft.print(F("AQUA MATE MONITOR"));

  // Left Card: Celsius Panel
  tft.drawRoundRect(10, 48, 145, 145, 8, ILI9341_CYAN);
  tft.drawRoundRect(11, 49, 143, 143, 7, ILI9341_DARKCYAN);
  tft.setTextColor(ILI9341_CYAN);
  tft.setTextSize(1);
  tft.setCursor(32, 62);
  tft.print(F("CELSIUS (C)"));
  tft.drawFastHLine(20, 78, 125, ILI9341_DARKGREY);

  // Right Card: Fahrenheit Panel
  tft.drawRoundRect(165, 48, 145, 145, 8, ILI9341_BLUE);
  tft.drawRoundRect(166, 49, 143, 143, 7, ILI9341_NAVY);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(182, 62);
  tft.print(F("FAHRENHEIT (F)"));
  tft.drawFastHLine(175, 78, 125, ILI9341_DARKGREY);

  // Status Footer Bar
  tft.drawRoundRect(10, 200, 300, 32, 6, ILI9341_GREEN);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(1);
  tft.setCursor(55, 212);
  tft.print(F("SYSTEM STATUS: NORMAL [ACTIVE]"));
}

void updateTFTDisplay(float tempC, float tempF, SensorStatus status) {
  // Clear value bounding areas to eliminate flickering and old string ghosting
  tft.fillRect(15, 95, 135, 85, ILI9341_BLACK);
  tft.fillRect(170, 95, 135, 85, ILI9341_BLACK);

  if (status != STATUS_OK) {
    tft.fillRect(10, 200, 300, 32, ILI9341_BLACK);
    tft.drawRoundRect(10, 200, 300, 32, 6, ILI9341_RED);
    tft.setTextColor(ILI9341_RED);
    tft.setTextSize(1);
    tft.setCursor(50, 212);

    if (status == STATUS_DISCONNECTED) {
      tft.print(F("ERROR: SENSOR DISCONNECTED (D27)"));
    } else {
      tft.print(F("ERROR: SENSOR OUT OF BOUNDS"));
    }

    tft.setTextColor(ILI9341_RED);
    tft.setTextSize(2);
    tft.setCursor(32, 125);
    tft.print(F("OFFLINE"));
    tft.setCursor(187, 125);
    tft.print(F("OFFLINE"));
    return;
  }

  // Restore status bar if recovered from error
  tft.fillRect(10, 200, 300, 32, ILI9341_BLACK);
  tft.drawRoundRect(10, 200, 300, 32, 6, ILI9341_GREEN);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(1);
  tft.setCursor(55, 212);
  tft.print(F("SYSTEM STATUS: NORMAL [ACTIVE]"));

  // Celsius Value Dynamic Color
  uint16_t cColor = ILI9341_GREEN;
  if (tempC < 20.0) cColor = ILI9341_CYAN;
  else if (tempC > 32.0) cColor = ILI9341_RED;

  // Render Celsius Readout
  tft.setTextColor(cColor);
  tft.setTextSize(3);
  tft.setCursor(20, 115);
  tft.print(tempC, 2);
  tft.setTextSize(2);
  tft.print(F(" C"));

  // Render Fahrenheit Readout
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);
  tft.setCursor(175, 115);
  tft.print(tempF, 1);
  tft.setTextSize(2);
  tft.print(F(" F"));
}