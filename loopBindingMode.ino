/**
 * ==============================================================================
 * loopBindingMode.ino (Controller - Instrumented Loop Status)
 * ==============================================================================
 */
void loopBindingMode() {
  static unsigned long lastBindSend = 0;
  static unsigned long lastDebugStatus = 0;
  
  displayMsg("BINDING...");

  // Print a heartbeat status every second so we know the loop is running and tracking flags
  if (millis() - lastDebugStatus > 1000) {
    lastDebugStatus = millis();
    Serial.printf("[LOOP-BIND] Controller searching... ReceiveBindingComplete flag = %d\n", ReceiveBindingComplete);
  }

  // Broadcast bind request packets (protocolMode = 0) every 200ms
  if (millis() - lastBindSend > 200) {
    lastBindSend = millis();
    
    RC_Packet_Structure bindPacket;
    memset(&bindPacket, 0, sizeof(bindPacket));
    bindPacket.protocolMode = 0; 
    
    Serial.println("[BIND] Broadcasting pairing request to car...");
    esp_now_send(broadcastMac, (uint8_t *)&bindPacket, sizeof(bindPacket));
  }

  if (ReceiveBindingComplete) {
    Serial.println("[SUCCESS] Binding handshake confirmed by car! Rebooting into Normal Mode...");
    delay(1000);
    ESP.restart();
  }

  delay(50);
}