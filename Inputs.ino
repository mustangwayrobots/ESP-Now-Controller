// ==============================================================================
// Inputs.ino - Clean ADC Reading, Calibration, and Normalization
// ==============================================================================

void readInputs() {    
    MenuButton   = (digitalRead(MENU_BUTTON_PIN) == LOW); 
    SelectButton = (digitalRead(SELECT_BUTTON_PIN) == LOW); 
    UpButton     = (digitalRead(UP_BUTTON_PIN) == LOW); 
    DownButton   = (digitalRead(DOWN_BUTTON_PIN) == LOW); 
    SpinButton   = (digitalRead(SEL_BUTTON_PIN) == LOW);   

    // Read raw 12-bit ADC values (0 to 4095 on ESP32)
    int rawY = constrain(analogRead(JOYSTICK_HORZ_PIN), 0, 4095); 
    int rawX = constrain(analogRead(JOYSTICK_VERT_PIN), 0, 4095);
    
    // Match index 10 to X and index 11 to Y
    int zeroX = 2048 + currentSettings[10]; 
    int zeroY = 2048 + currentSettings[11];

    // Fallback to absolute center (2048) if uncalibrated
    if (zeroX == 0) zeroX = 2048;
    if (zeroY == 0) zeroY = 2048;

    // Normalize raw ADC around the calibrated zero point into -1023 to 1023 range
    if (rawY >= zeroY) {
        joystickY = map(rawY, zeroY, 4095, 0, 1023);
    } else {
        joystickY = map(rawY, 0, zeroY, -1023, 0);
    }

    if (rawX >= zeroX) {
        joystickX = map(rawX, zeroX, 4095, 0, 1023);
    } else {
        joystickX = map(rawX, 0, zeroX, -1023, 0);
    }
    joystickX = constrain(joystickX, -1023, 1023);
    joystickY = constrain(joystickY, -1023, 1023);

    // DEBUG: Prints raw vs normalized joystick values to monitor axis behavior
    Serial.printf("[IN] rawX:%4d rawY:%4d | joyX:%4d joyY:%4d\n", rawX, rawY, joystickX, joystickY);
}