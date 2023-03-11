// Test for NodeMCU ESP8266 using a Pimorino 1.3" ST7789 240x240 Display
// Target for location
// "A" for orientation
// Cycles Colors for correct RGB
// Requires the correct pin connections and settings to offset "x" by 40
// can be used with Adafruit_STT7789.h or Bodmer TFT_eSPI.h libraries
// Use Setup24_PimoRound for Bodmer
//
// ************* Display Setup for Adafruit Librbaries
// defined(ESP8266)
// #define TFT_CS   D8
// #define TFT_RST  D4                                            
// #define TFT_DC   D3
// #define TFT_MOSI D7  // Data out
// #define TFT_SCLK D5  // Clock out
// Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
//
// NOTE -> Offset is not resolved for Adafruit Libaries

#include "SPI.h"
#include "TFT_eSPI.h"
#include <Adafruit_GFX.h>    // Core graphics library
//#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
TFT_eSPI tft = TFT_eSPI();

unsigned long total = 0;
unsigned long tn = 0;
void setup() {
  Serial.begin(115200);

  while (!Serial);
// Pimo Display Setup
  tft.init();
  tft.setRotation(0);
  tft.setViewport(0, 40, 240, 240);
//
  Serial.println(""); Serial.println("");
  Serial.println("TFT_eSPI 7789 PIMO Display Test - y Adjusted 40");

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(40, 120);
  tft.println("PIMO 7789 TEST");
  delay(2000);
  tft.fillScreen(TFT_BLACK);
  tft.drawCircle(120, 120, 25, TFT_WHITE);
  tft.drawLine(120, 150, 120, 90, TFT_WHITE);
  tft.drawLine(90, 120, 150, 120, TFT_WHITE);
  tft.fillCircle(120, 120, 20, TFT_BLACK);
  tft.setTextSize(4);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(110,105);
  tft.println("A");
  delay(10000);
  tft.fillScreen(TFT_BLACK);
  delay(2000);
  tft.fillCircle(120, 120, 60, TFT_GREEN);
  delay(2000);
  tft.fillScreen(TFT_BLACK);
  delay(2000);
  tft.fillCircle(120,120,120, TFT_BLUE);
  delay(2000);
  tft.fillCircle(120,120,80, TFT_WHITE);
  delay(2000);
  tft.fillCircle(120,120,20, TFT_RED);
  delay(2000);
}
int colorNum = 1;

void loop() {

if (colorNum == 1) {
  tft.fillScreen(TFT_RED);
} else if (colorNum == 2) {
  tft.fillScreen(TFT_GREEN);
} else if (colorNum == 3) {
  tft.fillScreen(TFT_BLUE);
} else if (colorNum == 4) {
  tft.fillScreen(TFT_YELLOW);
} else if (colorNum == 5) {
  tft.fillScreen(TFT_PURPLE);
} else if (colorNum == 6) {
  tft.fillScreen(TFT_WHITE);
} else if (colorNum == 7) {
  colorNum = 0;
}

colorNum = colorNum + 1;
Serial.println(colorNum);
delay(2000);

}