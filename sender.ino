#include <esp_now.h>
#include "esp_wifi.h"
#include <WiFi.h>
#include <M5StickCPlus.h>
#include "ui.h"

#define BUZZER_PIN 2

// DEFINE CHANNEL FOR SENDING
#define ESPNOW_CHANNEL 1


//-------------------- ESP-NOW SETUP -------------------- // 

// MAC ADDRESS OF RECEIVER ESP32
static const uint8_t receiverMAC[] = { X, X, X, X, X, X };

// STRUCTURE THAT IS SENT TO RECEIVER
typedef struct struct_message {
  bool powerButton;
  int speedButton;
} struct_message;

// ASSIGN INITIAL VALUES OF STRUCTURE
struct_message myData = { false, 0 };

// PMK KEY FOR ENCRYPTION
static const uint8_t PMK[ESP_NOW_KEY_LEN] = {
  'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'
};
//-------------------- TIMER SETUPS -------------------- // 

// BATTERY TIMER VALUES
unsigned long lastBatteryUpdate = 0;
const unsigned long batteryUpdateInterval = 1500;

// KEEP ALIVE TIMER VALUES
unsigned long lastSendTime = 0;
const unsigned long keepAliveInterval = 2000;

// AUTO POWER-OFF TIMER (10 MINS) VALUES
unsigned long lastOnTime = 0;
const unsigned long autoPowerInterval = 600000;

//-------------------- FLAGS AND VALUES -------------------- // 

bool powerButtonState = false;
int speedButtonValue = 0;
bool trailerPower = true;
bool lowBatteryWarningTriggered = false;


//----------------------- FUNCTIONS ------------------------ // 

// CALLBACK FUNCTION CALLED WHEN ESP-NOW DATA IS SENT
void dataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {

  // STORE PREVIOUS STATE OF TRAILER POWER
  static bool lastTrailerPower = true;

  // IF ESP-NOW FAILS AND RECEIVER IS ON, TURN RECEIVER OFF AND RESET CONTROL VALUES
  if (status != ESP_NOW_SEND_SUCCESS) {
    if (lastTrailerPower) {
      trailerPower = false;
      lastTrailerPower = false;
      powerButtonState = false;
      speedButtonValue = 0;

      // SEND UPDATED POWER AND SPEED VALUES TO RECEIVER
      myData.powerButton = powerButtonState;
      myData.speedButton = speedButtonValue;
      sendData();
      displayError();
    }

  // IF ESP-NOW SUCEEDS AND TRAILER IS ON, UPDATE LAST TRAILER POWER FLAG AND INITIALIZE UI
  } else {
    if (!lastTrailerPower) {
      trailerPower = true;
      lastTrailerPower = true;
      uiSetup();
      displayBattery(M5.Axp.GetBatVoltage());
      displayPowerValue(powerButtonState);
      displaySpeedValue(speedButtonValue);
    }
  }
}

// SETUP (CALLED ONCE, EACH TIME M5 TURNS ON)
void setup() {
  Serial.begin(115200);

  // INITIALIZES M5
  M5.begin();
  
  // INITIALIZES UI
  uiSetup();
  displayBattery(M5.Axp.GetBatVoltage());
  displayPowerValue(powerButtonState);
  displaySpeedValue(speedButtonValue);

  // SETS ESP-NOW MODE
  WiFi.mode(WIFI_MODE_STA);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  // INITIALIZE ESP-NOW
  if (esp_now_init() != ESP_OK) {
    return;
  }

  // INSTALL PMK
  if (esp_now_set_pmk(const_cast<uint8_t*>(PMK)) != ESP_OK) {
    return;
  }

  // REGISTER CALLBACK FUNCTION WITH NEW SIGNATURE
  esp_now_register_send_cb(dataSent);

  // COPY RECEIVER MAC ADDRESS AND ENABLE ENCRYPTION
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.encrypt = true;  
  memcpy(peerInfo.lmk, PMK, ESP_NOW_KEY_LEN);

  // ADDS RECEIVER
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    return;
  }
}

// SENDS ESP-NOW MESSAGE TO RECEIVER
void sendData() {
  esp_now_send(receiverMAC, (uint8_t *)&myData, sizeof(myData));
}

// LOOP (CALLED INDEFINITELY ON SENDER)
void loop() {

  // KEEPS M5 UP TO DATE
  M5.update();

  //--------------- M5 BATTERY --------------//

  // GET M5 BATTERY VOLTAGE
  float batteryVoltage = M5.Axp.GetBatVoltage();

  // UPDATES BATTERY EVERY 1500 ms
  if (millis() - lastBatteryUpdate >= batteryUpdateInterval) {
    lastBatteryUpdate = millis();
    displayBattery(batteryVoltage);
  }

  // LOW BATTERY WARNING (ONLY TRIGGERED ONCE PER DRAIN CYCLE)
  if (batteryVoltage <= 3.30 && batteryVoltage >= 3.29 && !lowBatteryWarningTriggered) {

    // UPDATES FLAG
    lowBatteryWarningTriggered = true;

    // UPDATES UI
    lowBatteryIndicator();
    uiSetup();
    displayPowerValue(powerButtonState);
    displaySpeedValue(speedButtonValue);
  }
  if (batteryVoltage > 4.10) {
    lowBatteryWarningTriggered = false;
  }

  // CONTROL LOOP (ONLY WHEN RECEIVER IS ON AND WITHIN RANGE)
  if (trailerPower) {

    // CONTINUE ONLY IF POWER BUTTON IS PRESSED
    if (M5.BtnB.wasPressed()) {

      // FLIP POWER STATE AND ASSIGN IT TO ESP-NOW MESSAGE STRUCT
      powerButtonState = !powerButtonState;
      myData.powerButton = powerButtonState;

      // RESET SPEED VALUE TO 0 AND UPDATE LAST TIME ON
      if (!powerButtonState) {
        speedButtonValue = 0;
        // UPDATE LAST TIME ON
        lastOnTime = millis();
      }

      // ASSIGN SPEED VALUE TO ESP-NOW MESSAGE STRUCT AND SEND IT
      myData.speedButton = speedButtonValue;
      sendData();

      // UPDATE UI
      displayPowerValue(powerButtonState);
      displaySpeedValue(speedButtonValue);
      tone(BUZZER_PIN, 2000, 75);
    }

    // CONTINUE ONLY IF SPEED BUTTON IS PRESSED AND POWER IS ON
    if (M5.BtnA.wasPressed() && powerButtonState) {

      // ITERATE FROM 0 TO 3 SPEED VALUES, ASSIGN TO ESP-NOW MESSAGE STRUCT, SEND DATA
      if (speedButtonValue < 3) {
        speedButtonValue++;
      } else {
        speedButtonValue = 0;
      }
      myData.speedButton = speedButtonValue;
      sendData();

      // UPDATE UI
      displaySpeedValue(speedButtonValue);
      tone(BUZZER_PIN, 4000, 75);
    }
  }

  // KEEP ALIVE CONDITIONAL (SENDS DATA TO RECEIVER EVERY TWO SECONDS)
  if (millis() - lastSendTime >= keepAliveInterval) {
    lastSendTime = millis();
    sendData();
  }

  // AUTO POWER OFF CONDITIONAL (M5 TURNS OFF AFTER 10 MIN IDLE)
  if ((millis() - lastOnTime >= autoPowerInterval) && (powerButtonState == false)) {
    M5.Axp.PowerOff();
  }
}
