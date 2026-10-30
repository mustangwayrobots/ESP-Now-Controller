// ==============================================================================
// loopNormalMode.ino - Normal Driving Loop & Connection State Management
// ==============================================================================

// Reference external send-ack watchdog tracker from setupNormalMode.ino
extern unsigned long lastSendAckTime;

void loopNormalMode() {
  // 1. ALWAYS read inputs first so button states are fresh on every loop cycle
  readInputs();  

  // 2. Check the menu next. If active, run ONLY the menu and stop everything else.
  if (ActiveMenu() == true) {
    delay(10); 
    return;    
  }

  // 3. --- SEND-ACK CONNECTION WATCHDOG ---
  // If we haven't received a successful send acknowledgment in > 2000ms (2 seconds), drop connection.
  // Otherwise, we are considered solidly connected based on our 20Hz control packet stream.
  if (millis() - lastSendAckTime > 1000) {
    TXConnectionStatus = false;
  } else {
    TXConnectionStatus = true;
  }

  // 4. Sound periodic beeps and handle display states based on real-time link status
  if (TXConnectionStatus == false) {
    Beep(1000, 10000, 100); // 1000Hz tone, 10-second period, 100ms duration
    displayMsg("WAITING");
  } else {
    // Convert millivolts to Volts (e.g., 4100 -> 4.1V)
    float voltage = ReceivingData.battery / 1000.0;

    // Stable two-line format: Line 1 = CONN, Line 2 = Bat=X.X
    char displayBuffer[32];
    snprintf(displayBuffer, sizeof(displayBuffer), "CONN Bat=%.1fV", voltage);
    displayMsg(displayBuffer);
  }

  // 5. --- Normal Driving Telemetry ---
  delay(1);     
  CalculateMotorDriverValues(joystickX, joystickY);
  SendData();   
  
  delay(20); 
}