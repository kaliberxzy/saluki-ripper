#ifndef UI
#define UI

// SETUP
void uiSetup();

// POWER
void displayPowerValue(bool powerValue);

// SPEED
void displaySpeedValue(int speedValue);

// BATTERY
void displayBattery(float batteryVoltage);
void lowBatteryIndicator();

// ERROR
void displayError();

#endif