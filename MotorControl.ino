/**
 * ==============================================================================
 * MotorControl.ino - Clean Arcade Drive Mixing, Deadband, and Hardware Mapping
 * ==============================================================================
 */

void CalculateMotorDriverValues(int rawX, int rawY) {
  // --- 1. AXIS MAPPING ---
  int throttle = rawY;
  int steering = rawX;

  // --- 2. EXTRACT CONFIGURATION SETTINGS ---
  int MaxSpeed   = map(currentSettings[0], 1, 127, 0, 255); 
  int Trim       = currentSettings[1]; 
  int DeadZone   = currentSettings[12];
  int SteerRate  = map(currentSettings[3], 1, 127, 0, 255);

  // --- 3. APPLY DEADZONE ---
  if (abs(throttle) < DeadZone) throttle = 0;
  if (abs(steering) < DeadZone) steering = 0;

  // --- 4. SCALE TO SIGNED 8-BIT SPEED RANGES ---
  int throttleVal = map(throttle, -1023, 1023, -MaxSpeed, MaxSpeed);
  int steeringVal = map(steering, -1023, 1023, -SteerRate, SteerRate);

  // --- 5. STANDARD ARCADE DRIVE MIXING ---
  // Left motor = throttle + steering | Right motor = throttle - steering
  int rawLeft  = throttleVal + steeringVal;
  int rawRight = throttleVal - steeringVal;

  // Apply Asymmetric Trim
  if (Trim > 0) {
    rawLeft = constrain(rawLeft - Trim, -255, 255);
  } else if (Trim < 0) {
    rawRight = constrain(rawRight + Trim, -255, 255);
  }

  // Constrain to valid 8-bit limits
  rawLeft  = constrain(rawLeft, -255, 255);
  rawRight = constrain(rawRight, -255, 255);

  // --- 6. JITTER FLOOR (Eliminates H-bridge clicking at limits) ---
  if (abs(rawLeft) < 15)  rawLeft = 0;
  if (abs(rawRight) < 15) rawRight = 0;

  // --- 7. DUAL-PIN PWM CONVERSION (Forward / Reverse Pins) ---
  int a1 = 0, a2 = 0, b1 = 0, b2 = 0;

  // Motor A (Left)
  if (rawLeft > 0) {
    a1 = rawLeft;  a2 = 0;      
  } else if (rawLeft < 0) {
    a1 = 0;        a2 = abs(rawLeft); 
  }

  // Motor B (Right)
  if (rawRight > 0) {
    b1 = rawRight; b2 = 0;      
  } else if (rawRight < 0) {
    b1 = 0;        b2 = abs(rawRight); 
  }

  // Assign to ESP-NOW packet structure
  SendingData.motor_A1 = a1;
  SendingData.motor_A2 = a2;
  SendingData.motor_B1 = b1;
  SendingData.motor_B2 = b2;

  // DEBUG: Print what is actually being sent to the car motors
  Serial.printf("[MOTOR] Left(A1:%3d A2:%3d) | Right(B1:%3d B2:%3d)\n", a1, a2, b1, b2);

  // --- 8. HARDWARE CONFIGURATION OVERRIDES ---
  int tempHolding;
  if (currentSettings[4] != 0){ // Flip Motor A
    tempHolding = SendingData.motor_A1;
    SendingData.motor_A1 = SendingData.motor_A2;
    SendingData.motor_A2 = tempHolding;
  }
  if (currentSettings[5] != 0){ // Flip Motor B
    tempHolding = SendingData.motor_B1;
    SendingData.motor_B1 = SendingData.motor_B2;
    SendingData.motor_B2 = tempHolding;
  }
  if (currentSettings[6] != 0){ // Swap Motors A & B
    tempHolding = SendingData.motor_A1;
    SendingData.motor_A1 = SendingData.motor_B1; 
    SendingData.motor_B1 = tempHolding; 
    tempHolding = SendingData.motor_A2;
    SendingData.motor_A2 = SendingData.motor_B2; 
    SendingData.motor_B2 = tempHolding; 
  }

  // --- 9. WEAPON STATE HANDLING ---
  if(UpButton == false) SendingData.weapon = currentSettings[9];
  if(UpButton == true) SendingData.weapon = currentSettings[8];

  SendData(); 
}

void SendData(){
    SendingData.protocolMode = 1; // Normal Drive Mode
    uint8_t TempData[sizeof(SendingData)];
    memcpy(TempData, &SendingData, sizeof(SendingData));
    
    uint8_t remoteMac[6];
    if (readStoredMac(remoteMac)) {
      esp_now_send(remoteMac, TempData, sizeof(TempData));        
    }
}