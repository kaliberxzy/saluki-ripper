#include <esp_now.h>
#include "esp_wifi.h"
#include <WiFi.h>
#include <AccelStepper.h>

// DEFINE CHANNEL FOR SENDING
#define ESPNOW_CHANNEL 1

// DEFINE STEPPER MOTOR DRIVER PINS
#define STEP_PIN_1 22   
#define DIR_PIN_1 23 
#define ENABLE_PIN_1 15

#define STEP_PIN_2 20 
#define DIR_PIN_2 21  
#define ENABLE_PIN_2 19

#define DRIVER_ON 18

// DEFINE LED PINS
#define ON_LED 4
#define P1 5
#define P2 6
#define P3 7

// DEFINE POSITIONS OF STEPPER MOTORS
#define STEPPER_1_POS_1 150    // Position for speed 1
#define STEPPER_1_POS_2 225  // Position for speed 2
#define STEPPER_1_POS_3 300    // Position for speed 3
#define STEPPER_1_HOME_POS 0  // Default home position

#define STEPPER_2_POS_1 -150    // Position for speed 1
#define STEPPER_2_POS_2 -225  // Position for speed 2
#define STEPPER_2_POS_3 -300    // Position for speed 3
#define STEPPER_2_HOME_POS 0   // Default home position

// PMK KEY FOR DECRYPTION
static const uint8_t PMK[ESP_NOW_KEY_LEN] = {
  'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'
};

// SENDER MAC ADDRESS
static const uint8_t senderMAC[6] = { X, X, X, X, X, X };

// STRUCTURE THAT IS RECIVED BY SENDER (ANY DATA THAT IS TRANSMITTED)
typedef struct struct_message {
  bool onButton;
  int speedButton;
} struct_message;

// ASSIGN INITIAL POWER AND SPEED VALUES FOR ESP-NOW STRUCTURE
volatile struct_message receivedCommand = { false, 0 };
volatile bool newCommandReceived = false;

// ASSIGN INITIAL POWER AND SPEED VALUES FOR RECEIVER ESP32
bool powerButtonState = false;
int speedButtonValue = 0;

// ASSIGN VALUES FOR KEEP ALIVE MECHANISM
unsigned long lastReceivedTime = 0;
const unsigned long keepAliveInterval = 4000;  

// INITIALIZE STEPPER MOTORS
AccelStepper stepper1(AccelStepper::DRIVER, STEP_PIN_1, DIR_PIN_1);
AccelStepper stepper2(AccelStepper::DRIVER, STEP_PIN_2, DIR_PIN_2);

// CALLBACK FUNCTION CALLED WHEN ESP-NOW DATA IS RECEIVED
void dataReceived(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {

  // COPIES DATA INTO STRUCTURE AND SETS NEW_COMMAND_RECEIVED FLAG TO TRUE
  memcpy((void *)&receivedCommand, data, sizeof(receivedCommand));
  newCommandReceived = true;
}

// SETUP (CALLED ONCE, EACH TIME RECEIVER TURNS ON)
void setup() {
  Serial.begin(115200);

  // SETS CONTROL PINS AS OUTPUT
  pinMode(STEP_PIN_1, OUTPUT);
  pinMode(DIR_PIN_1, OUTPUT);
  pinMode(ENABLE_PIN_1, OUTPUT);

  pinMode(STEP_PIN_2, OUTPUT);
  pinMode(DIR_PIN_2, OUTPUT);
  pinMode(ENABLE_PIN_2, OUTPUT);

  pinMode(DRIVER_ON, OUTPUT);

  pinMode(ON_LED, OUTPUT);
  pinMode(P1, OUTPUT);
  pinMode(P2, OUTPUT);
  pinMode(P3, OUTPUT);

  // ENABLE STEPPER DRIVERS (ACTIVE = LOW)
  digitalWrite(ENABLE_PIN_1, LOW);
  digitalWrite(ENABLE_PIN_2, LOW);
  digitalWrite(DRIVER_ON, LOW);

  // INITIALIZE LED PINS
  digitalWrite(ON_LED, LOW);
  digitalWrite(P1, LOW);
  digitalWrite(P2, LOW);
  digitalWrite(P3, LOW);

  // DEFINES STEPPER MOTOR MAX SPEED AND ACCELERATION
  stepper1.setMaxSpeed(1000);
  stepper1.setAcceleration(3000);
  stepper2.setMaxSpeed(1000);
  stepper2.setAcceleration(3000);

  // DEFINES STEPPER MOTORS HOME POSITION
  stepper1.setCurrentPosition(STEPPER_1_HOME_POS);
  stepper2.setCurrentPosition(STEPPER_2_HOME_POS);

  WiFi.mode(WIFI_MODE_STA);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  // INITIALIZE ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed!");
    return;
  }

  // INSTALL SHARED PMK FOR ENCRYPTION
  if (esp_now_set_pmk(const_cast<uint8_t*>(PMK)) != ESP_OK) {
    Serial.println("esp_now_set_pmk failed");
    return;
  }

  // REGISTER CALLBACK FUNCTION WITH NEW SIGNATURE
  esp_now_register_recv_cb(dataReceived);

  // REGISTER SENDER AS ENCRYPTED PEER
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, senderMAC, 6);
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.encrypt = true;
  memcpy(peerInfo.lmk, PMK, ESP_NOW_KEY_LEN);

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add encrypted peer");
    return;
  }
}

// LOOP (CALLED INDEFINITELY ON RECEIVER)
void loop() {

  // EXTRACTS AND ASSIGNS POWER AND SPEED VALUES FROM RECEIVED DATA
  powerButtonState = receivedCommand.onButton;
  speedButtonValue = receivedCommand.speedButton;

  // CONTINUE ONLY IF A NEW COMMAND IS RECEIVED
  if (newCommandReceived) {

    // RESET RECEIVED FLAG
    newCommandReceived = false;

    // UPDATE RECEIVED TIMESTAMP (FOR KEEP ALIVE)
    lastReceivedTime = millis();

    // CONTINUE ONLY IF POWER BUTTON IS PRESSED (TRUE) ON M5
    if (powerButtonState) {

      // TURN ON STEPPER MOTORS AND LED
      digitalWrite(DRIVER_ON, HIGH);
      digitalWrite(ON_LED, HIGH);

      // MOVE STEPPER MOTOR BASED ON SPEED VALUE
      if (speedButtonValue == 1) {
        stepper1.moveTo(STEPPER_1_POS_1);
        stepper2.moveTo(STEPPER_2_POS_1);
        digitalWrite(P1, HIGH);
        digitalWrite(P2, LOW);
        digitalWrite(P3, LOW);

      } else if (speedButtonValue == 2) {
        stepper1.moveTo(STEPPER_1_POS_2);
        stepper2.moveTo(STEPPER_2_POS_2);
        digitalWrite(P1, HIGH);
        digitalWrite(P2, HIGH);
        digitalWrite(P3, LOW);

      } else if (speedButtonValue == 3) {
        stepper1.moveTo(STEPPER_1_POS_3);
        stepper2.moveTo(STEPPER_2_POS_3);
        digitalWrite(P1, HIGH);
        digitalWrite(P2, HIGH);
        digitalWrite(P3, HIGH);

      // IF SPEED = 0, MOVE STEPPER MOTORS BACK TO HOME POSITION
      } else {
        stepper1.moveTo(STEPPER_1_HOME_POS);
        stepper2.moveTo(STEPPER_2_HOME_POS);

        // ENSURES STEPPER MOTORS ARE ON WHILE MOVING HOME
        while (stepper1.distanceToGo() != 0 || stepper2.distanceToGo() != 0) {
          stepper1.run();
          stepper2.run();
        }
        digitalWrite(P1, LOW);
        digitalWrite(P2, LOW);
        digitalWrite(P3, LOW);
      }

    // IF POWER = FALSE, MOVE STEPPER MOTORS BACK TO HOME POSITION
    } else {
      stepper1.moveTo(STEPPER_1_HOME_POS);
      stepper2.moveTo(STEPPER_2_HOME_POS);

      // ENSURES STEPPER MOTORS ARE ON WHILE MOVING HOME
      while (stepper1.distanceToGo() != 0 || stepper2.distanceToGo() != 0) {
        stepper1.run();
        stepper2.run();
      }
      digitalWrite(P1, LOW);
      digitalWrite(P2, LOW);
      digitalWrite(P3, LOW);

      // TURNS OFF STEPPER MOTORS
      stepper1.stop();
      stepper2.stop();
      digitalWrite(DRIVER_ON, LOW);
      digitalWrite(ON_LED, LOW);
    }
  }

  if (stepper1.distanceToGo() != 0) {
    stepper1.run();
  }
  if (stepper2.distanceToGo() != 0) {
    stepper2.run();
  }

  // KEEP ALIVE
  if (millis() - lastReceivedTime >= keepAliveInterval) {

    stepper1.moveTo(STEPPER_1_HOME_POS);
    stepper2.moveTo(STEPPER_2_HOME_POS);

    while (stepper1.distanceToGo() != 0 || stepper2.distanceToGo() != 0) {
      stepper1.run();
      stepper2.run();
    }
    digitalWrite(P1, LOW);
    digitalWrite(P2, LOW);
    digitalWrite(P3, LOW);

    stepper1.stop();
    stepper2.stop();
    digitalWrite(ON_LED, LOW);
    digitalWrite(DRIVER_ON, LOW);
  }
}
