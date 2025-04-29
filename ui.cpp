#include "ui.h"
#include <M5StickCPlus.h>

#define BUZZER_PIN 2

// SETUP
void uiSetup() {

  // DISPLAY SETUP
  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.fillRect(135, 0, 1, 140, WHITE);

  // TEXT SETUP
  M5.Lcd.setTextSize(0);
  M5.Lcd.setTextColor(DARKGREY);
  M5.Lcd.setFreeFont(&FreeSans9pt7b);

  // POWER LABEL
  M5.Lcd.setCursor(33, 26);
  M5.Lcd.print("POWER");

  // SPEED LABEL
  M5.Lcd.setCursor(157, 26);
  M5.Lcd.print("SPEED");
}

// POWER
void displayPowerValue(bool powerValue) {
  if (powerValue) {
    M5.Lcd.setTextSize(0);
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.setFreeFont(&FreeSansBold18pt7b);
    M5.Lcd.setCursor(40, 72);
    M5.Lcd.fillRect(20, 30, 80, 50, BLACK);
    M5.Lcd.print("ON");
  } else {
    M5.Lcd.setTextSize(0);
    M5.Lcd.setTextColor(RED);
    M5.Lcd.setFreeFont(&FreeSansBold18pt7b);
    M5.Lcd.setCursor(31, 72);
    M5.Lcd.fillRect(20, 30, 80, 50, BLACK);
    M5.Lcd.print("OFF");
  }
}

// SPEED
void displaySpeedValue(int speedValue) {
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setFreeFont(&FreeSansBold24pt7b);
  M5.Lcd.setCursor(162, 114);
  M5.Lcd.fillRect(140, 30, 80, 100, BLACK);
  M5.Lcd.print(speedValue);
}

// BATTERY
void displayBattery(float batteryVoltage) {
  
  // BATTERY SHELL
  M5.Lcd.fillRect(30, 90, 105, 30, BLACK);
  M5.Lcd.drawRoundRect(31, 90, 70, 28, 4, WHITE);
  M5.Lcd.drawRoundRect(100, 96, 4, 16, 1, WHITE);
  M5.Lcd.fillRect(100, 96, 3, 16, WHITE);

  if (M5.Axp.GetVBusVoltage() < 1) {
    M5.Axp.ScreenBreath(100);

    // MAX VOLTAGE = 4.15
    M5.Lcd.fillRect(35, 94, 10, 20, (batteryVoltage >= 3.14 ? WHITE : BLACK));
    M5.Lcd.fillRect(48, 94, 10, 20, (batteryVoltage >= 3.5 ? WHITE : BLACK));
    M5.Lcd.fillRect(61, 94, 10, 20, (batteryVoltage >= 3.65 ? WHITE : BLACK));
    M5.Lcd.fillRect(74, 94, 10, 20, (batteryVoltage >= 3.75 ? WHITE : BLACK));
    M5.Lcd.fillRect(87, 94, 10, 20, (batteryVoltage >= 3.85 ? WHITE : BLACK));
  } else {
    M5.Axp.ScreenBreath(25);
    M5.Lcd.fillTriangle(45, 102, 71, 102, 71, 112, WHITE);
    M5.Lcd.fillTriangle(61, 94, 87, 104, 61, 104, WHITE);
  }
}

void lowBatteryIndicator() {

    M5.Lcd.setTextSize(0);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setFreeFont(&FreeSansBold12pt7b);

    for (int i = 0; i < 3; i++) {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(32, 77);
      delay(250);
      M5.Lcd.print("LOW BATTERY");
      tone(BUZZER_PIN, 4000, 100);
      delay(250);
    }
}

// ERROR
void displayError() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(0);
  M5.Lcd.setFreeFont(&FreeSans9pt7b);
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setCursor(20, 27);
  M5.Lcd.print("ERROR CONNECTING.");
  M5.Lcd.setCursor(20, 55);
  M5.Lcd.print("ENSURE TRAILER IS");
  M5.Lcd.setCursor(20, 73);
  M5.Lcd.print("ON AND WITHIN RANGE");
}
