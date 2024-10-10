/**
 * The Pawn Shop Trinket
 * Copyright Leigh Klotz <klotz@klotz.me> 2024
 * Inspired by Blatano <https://github.com/leighklotz/blatano> but designed for The Pawn Shop, SF:
 * <https://www.forbes.com/sites/chelseadavis/2019/02/25/the-pawn-shop-a-secret-tapas-bar-in-san-franciscos-soma-neighborhood/>
 * In Arduino, enable "Turn on CDC on Boot"
 */
#include <Arduino.h>
#include <math.h>

#include "BLEUUID.h"
#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEScan.h"
#include "BLEAdvertisedDevice.h"

#include <WS2812FX.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
#define LED_COUNT 1
#define LED_PIN 39
#define SDA_PIN 41
#define SCL_PIN 40

#include "logo.h"

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);

const int LED_BRIGHTNESS = 2;
const int DISPLAY_BRIGHTNESS = 12;

// pixel jitter +/- 1 to reduce OLED burn-in
int jitter_count = 0;

// Macro to get jittered coordinates
#define J(x,y) (x + jitter_x()), (y + jitter_y())
#define jitter_x() ((jitter_count % 3) - 1)
#define jitter_y() (((jitter_count / 3) % 3) - 1)

U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); // EastRising 0.42" OLED

BLEScan* pBLEScan;
int scanTime = 5;  // BLE scan time in seconds


void selfIdentify() {
  Serial.println("ThePawnShopTrinket");
  Serial.println("<https://github.com/leighklotz/ThePawnShopTrinket>");
  Serial.println("Copyright Leigh Klotz <klotz@klotz.me> 2024");
}

void setup() {
  Serial.begin(9600);
  selfIdentify();

  led_init();
  u8g2_init();
  ble_init();

  // Delay for a bit to show the red LED
  delay(2000);

  // Turn off the LED
  ws2812fx.setColor(0x000000);
  ws2812fx.service();
  Serial.println("Init Done");
}

void u8g2_init(void) {
  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();
  u8g2.setContrast(DISPLAY_BRIGHTNESS);
  setSmallFont();
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

void led_init(void) {
  // Initialize the LED
  ws2812fx.init();
  ws2812fx.setBrightness(LED_BRIGHTNESS);
  ws2812fx.setMode(FX_MODE_STATIC);
  // RED GREEN BLUE WHITE BLACK YELLOW CYAN MAGENTA PURPLE ORANGE PINK GRAY ULTRAWHITE DIM DARK	
  ws2812fx.setColor(RED);
  ws2812fx.start();
  ws2812fx.service();
}

void loop() {
  jitter();

  drawLogo();
  delay(2000);
  drawPage1();
  delay(2000);
  drawPage2();
  delay(3000);
}

// Update the jitter_count to cycle through jitter values
void jitter() {
  jitter_count = (jitter_count + 1) % 9;  // 9 = 3x3 (3 values for x, and 3 for y)
  if (jitter_count == 0) {
    selfIdentify();
  }
  Serial.printf("Jitter: %d,%d\n", jitter_x(), jitter_y());
}


void drawPage1() {
  u8g2.clearBuffer();

  // Display the message
  u8g2.drawStr(J(1, 1), "Welcome to the");
  u8g2.drawStr(J(1, 8), "Prohibition Era!");
  u8g2.drawStr(J(1, 16), "A time of speak-");
  u8g2.drawStr(J(1, 24), "easies, gangsters");
  u8g2.drawStr(J(1, 32), "and secrets.");

  u8g2.sendBuffer();
  ws2812fx.setColor(BLUE);
  ws2812fx.service();
}

void drawPage2() {
  u8g2.clearBuffer();

  // Get the number of BLE devices
  int deviceCount = scanBLEDevices();

  // Draw the number of BLE devices found centered on the screen
  // There are many magic constants here.
  {
    char s[32];
    setBigFont();
    snprintf(s, sizeof(s), "%d\n", deviceCount);
    const int line_height = 20;
    const int strWidth = u8g2.getStrWidth(s);
    const int xPos = (u8g2.getWidth() - strWidth) / 2;
    const int yPos = ((u8g2.getHeight() - line_height) / 2) + 1;

    // Calculate radius of the circle
    const int circleDiameter = max(strWidth + 4, 24);
    const int circleRadius = circleDiameter / 2;

    {
      // Parameters to control the circular path
      const int path_radius = 4; // This controls how far the circle bounces from the center
      const float angleIncrement = M_PI / 18; // How much to increase the angle for each step, in radians
      const int centerX = u8g2.getWidth() / 2; // Center of the screen along the X-axis
      const int centerY = u8g2.getHeight() / 2; // Center of the screen along the Y-axis

      float angle = 0; // Starting angle

      // Draw in a circular motion
      for (int i = 0; i < 36 * 6; ++i) { // 36 steps to complete a full circle
	// Calculate the circular path offsets
	int xo = centerX + int(path_radius * cos(angle));
	int yo = centerY + int(path_radius * sin(angle));

	u8g2.clearBuffer();

	// Draw the circle at the calculated positions
	u8g2.drawCircle(J(xo, yo), circleRadius);

	// Draw the string inside the circle
	u8g2.drawStr(J(xo - circleRadius/2 - strWidth/4, yo - circleRadius/2 - 1), s);

	u8g2.sendBuffer();

	angle += angleIncrement;

	delay(20);
      }
    }
    u8g2.clearBuffer();
    u8g2.sendBuffer();
  }
  setSmallFont();
  ws2812fx.setColor(MAGENTA);
  ws2812fx.service();
}

void drawLogo() {
  ws2812fx.setColor(RED);
  ws2812fx.service();
  u8g2.clearBuffer();
  u8g2.setBitmapMode(false /* solid */);
  u8g2.setDrawColor(1); // White
  const int xpos = (u8g2.getWidth() - logo_bitmap_width) / 2;
  u8g2.drawXBM(J(xpos, 1), logo_bitmap_width, logo_bitmap_height, logo_bitmap);
  u8g2.sendBuffer();
}

void ble_init() {
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();  // Initialize BLE scan
  pBLEScan->setActiveScan(true);    // Active scan mode
}

int scanBLEDevices() {
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);  // Start BLE scan
  int numDevices = foundDevices.getCount();  // Get the number of devices
  pBLEScan->clearResults();  // Clear results after scan
  return numDevices;
}

void setSmallFont() {
  u8g2.setFont(u8g2_font_tiny5_tf);
}

void setBigFont() {
  u8g2.setFont(u8g2_font_fur14_tn);
}
