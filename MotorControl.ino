/**
 * ==============================================================================
 * MotorControl.ino - Clean Arcade Drive, Spin-on-Demand, and Directional Trim
 * ==============================================================================
 */

void CalculateMotorDriverValues(int rawX, int rawY) {
  // --- 1. AXIS MAPPING ---
  int throttle = -rawY; 
  int steering = rawX;

  // --- 2. EXTRACT CONFIGURATION SETTINGS ---
  int MaxSpeed   = map(currentSettings[0], 1, 127, 0, 255); 
  int ForwardTrim = currentSettings[1]; // Your working forward trim (15)
  int DeadZone   = currentSettings[12];
  
  // Index 3: STEER RATE
  float steerRatePct = currentSettings[3] / 100.0; 
  // Index 4: STEER EXPO
  float steerExpoPct = currentSettings[4] / 100.0; 
  
  // Index 8: SPIN SPEED
  int spinSpeed = map(currentSettings[8], 0, 127, 0, 255);

  // --- 3. APPLY DEADZONE ---
  if (abs(throttle) < DeadZone) throttle = 0;
  if (abs(steering) < DeadZone) steering = 0;

  int rawLeft = 0;
  int rawRight = 0;

  // --- 4. SPIN-ON-DEMAND VS NORMAL ARCADE DRIVE ---
  if (SpinButton) {
    if (abs(steering) > DeadZone) {
      if (steering > 0) {
        rawLeft  = spinSpeed;
        rawRight = -spinSpeed;
      } else {
        rawLeft  = -spinSpeed;
        rawRight = spinSpeed;
      }
    } else {
      rawLeft  = 0;
      rawRight = 0;
    }
  } else {
    // Normal Arcade Drive with Rate & Expo
    int throttleVal = map(throttle, -1023, 1023, -MaxSpeed, MaxSpeed);

    // Normalize steering input to -1.0 to +1.0 float range
    float normSteer = steering / 1023.0;

    // Apply Cubic Exponential Curve
    float cubicSteer = normSteer * normSteer * normSteer;
    float blendedSteer = ((1.0 - steerExpoPct) * normSteer) + (steerExpoPct * cubicSteer);

    // Calculate Maximum Steering Cap
    int maxSteeringCap = (int)(MaxSpeed * steerRatePct);
    int steeringVal = (int)(blendedSteer * maxSteeringCap);

    // Standard Arcade Drive Mixing
    rawLeft  = throttleVal + steeringVal;
    rawRight = throttleVal - steeringVal;

    // --- DIRECTIONAL ASYMMETRIC TRIM ---
    int activeTrim = ForwardTrim;
    if (throttleVal < 0) {
      activeTrim = -ForwardTrim; // Fully reverses trim compensation for reverse motion
    }

    if (activeTrim > 0) {
      rawLeft = constrain(rawLeft - activeTrim, -255, 255);
    } else if (activeTrim < 0) {
      rawRight = constrain(rawRight + activeTrim, -255, 255);
    }
  }

  // Constrain to valid 8-bit limits
  rawLeft  = constrain(rawLeft, -255, 255);
  rawRight = constrain(rawRight, -255, 255);

  // --- 5. TRANSIENT INERTIA SMOOTHING (Ramp Filter) ---
  static int prevLeft = 0;
  static int prevRight = 0;
  int rampStep = 40; 
  
  rawLeft  = constrain(rawLeft, prevLeft - rampStep, prevLeft + rampStep);
  rawRight = constrain(rawRight, prevRight - rampStep, prevRight + rampStep);
  
  prevLeft = rawLeft;
  prevRight = rawRight;

  // --- 6. JITTER FLOOR ---
  if (abs(rawLeft) < 15)  rawLeft = 0;
  if (abs(rawRight) < 15) rawRight = 0;

  // --- 7. DUAL-PIN PWM CONVERSION ---
  int a1 = 0, a2 = 0, b1 = 0, b2 = 0;

  if (rawLeft > 0) {
    a1 = rawLeft;  a2 = 0;      
  } else if (rawLeft < 0) {
    a1 = 0;        a2 = abs(rawLeft); 
  }

  if (rawRight > 0) {
    b1 = rawRight; b2 = 0;      
  } else if (rawRight < 0) {
    b1 = 0;        b2 = abs(rawRight); 
  }

  SendingData.motor_A1 = a1;
  SendingData.motor_A2 = a2;
  SendingData.motor_B1 = b1;
  SendingData.motor_B2 = b2;

  // --- 8. HARDWARE CONFIGURATION OVERRIDES ---
  int tempHolding;
  if (currentSettings[5] != 0){ 
    tempHolding = SendingData.motor_A1;
    SendingData.motor_A1 = SendingData.motor_A2;
    SendingData.motor_A2 = tempHolding;
  }
  if (currentSettings[6] != 0){ 
    tempHolding = SendingData.motor_B1;
    SendingData.motor_B1 = SendingData.motor_B2;
    SendingData.motor_B2 = tempHolding;
  }
  if (currentSettings[7] != 0){ 
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