/**
 * The Pawn Shop Trinket
 * Copyright Leigh Klotz <klotz@klotz.me> 2024
 * Inspired by Blatano <https://github.com/leighklotz/blatano> but designed for The Pawn Shop, SF:
 * <https://www.forbes.com/sites/chelseadavis/2019/02/25/the-pawn-shop-a-secret-tapas-bar-in-san-franciscos-soma-neighborhood/>
 */
#include <Arduino.h>

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
const int DISPLAY_BRIGHTNESS = 10;

// todo: pixel jitter +/- 1 to reduce OLED burn-in
int x_jitter = 0;
int y_jitter = 0;
int x_counter = 0;
#define J(x,y) (x+x_jitter), (y+y_jitter)


// Hold down B button (non-LED side) on board, press R button (LED side) and release again
// but keep on pressing B (non-LED side), trigger Arduino IDE to upload sketch,
// keep B (non-LED side) pressed until Arduino IDE says that it's connected.
// Then you can release B button.(non-LED side) 


U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); // EastRising 0.42" OLED


BLEScan* pBLEScan;
int scanTime = 5;  // scan time in seconds


void setup() {
  Serial.begin(9600);
  Serial.println("ThePawnShopTrinket - Leigh Klotz <klotz@klotz.me>");

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

  Serial.println("<https://github.com/leighklotz/ThePawnShopTrinket>");
  Serial.println("Copyright Leigh Klotz <klotz@klotz.me> 2024");

  drawLogo();
  delay(2000);
  drawPage1();
  delay(2000);
  drawPage2();
  delay(3000);
}

// Update x_jitter and y_jitter to cycle through [-1, 0, 1]
void jitter() {
  x_jitter = (x_jitter + 1 + 1) % 3 - 1;

  x_counter = (x_counter + 1) % 3;

  if (x_counter == 0) {
    y_jitter = (y_jitter + 1 + 1) % 3 - 1;
  }

  Serial.printf("Jitter: %d,%d\n", J(0,0));
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
  {
    char s[32];
    setBigFont();
    snprintf(s, sizeof(s), "%d\n", deviceCount);
    const int line_height = 20;
    const int strWidth = u8g2.getStrWidth(s);
    const int xPos = (u8g2.getWidth() - strWidth) / 2;
    const int yPos = ((u8g2.getHeight() - line_height) / 2) + 1;

    // Calculate radius of the circle
    const int circleDiameter = strWidth + 10; // Add 10 to string width for slightly larger circle
    const int circleRadius = circleDiameter / 2;

    // Center the circle on the screen and adjust if needed to fit
    const int circleXPos = (u8g2.getWidth() - circleDiameter) / 2;
    const int circleYPos = (u8g2.getHeight() - circleDiameter) / 2;
  
    // Draw the circle
    u8g2.drawCircle(J(circleXPos + circleRadius, circleYPos + circleRadius), circleRadius);


    // Draw the string inside the circle
    u8g2.drawStr(J(xPos, yPos), s);
    u8g2.sendBuffer();
    setSmallFont();
  }

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

/* 
 * BLE Count
 */

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
  // u8g2_font_6x10_tf,  u8g2_font_5x7_tf, u8g2_font_tiny5_tf, u8g2_font_u8glib_4_tf
  u8g2.setFont(u8g2_font_tiny5_tf);
}

void setBigFont() {
  // u8g2_font_fur20_tn
  u8g2.setFont(u8g2_font_fur14_tn);
}
