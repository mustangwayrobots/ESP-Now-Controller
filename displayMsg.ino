// ==============================================================================
// displayMsg.ino - Display Message Routine with Contrast and Blink Control
// ==============================================================================
void displayMsg(const char *NewMessage, int contrast, int blinkMode) {
  static char currentMessage[50];
  static uint32_t blinkMillis = 0;
  static boolean displayState = true;
  char *orignal;
  char *token, *theRest;
  String myLine[5];
  
  // Apply hardware contrast setting
  u8g2.setContrast(contrast);

  // Handle blink duty cycle (blinkMode: 1 = solid ON, 2 = 50% duty, 3 = 33% duty, etc.)
// Handle blink duty cycle (blinkMode: 1 = solid ON, 2 = 1 second interval, etc.)
  if (blinkMode > 1) {
    unsigned long interval = 1000 * (blinkMode - 1); // 1 second per step
    if (millis() - blinkMillis > interval) {
      blinkMillis = millis();
      displayState = !displayState;
    }    
    // If duty cycle puts us in the "OFF" phase, force power save blanking
    if (!displayState) {
      u8g2.setPowerSave(1);
      return;
    }
  }
  
  // Ensure power save is off when displaying
  u8g2.setPowerSave(0);

  if(strcmp(currentMessage, NewMessage) != 0){
    strcpy(currentMessage, NewMessage);
    if (strlen(currentMessage) <= 9){
      u8g2.clearBuffer();          
      u8g2.setFont(u8g2_font_inb16_mr); 
      u8g2.drawStr(0, 24, currentMessage); 
      u8g2.sendBuffer();          

    }else{
      int count = 0;
      orignal = strdup(NewMessage);
      theRest = orignal;
      myLine[count] = "";
      while((token = strtok_r(theRest, " ", &theRest))){
        if (myLine[count].length() + strlen(token) < 9){
          if (myLine[count].length() > 0){
            myLine[count] += " ";          
          }
          myLine[count] += token;
        }else{
          count++;
          myLine[count] = String(token);          
        }
      } 
      u8g2.clearBuffer();          
      u8g2.setFont(u8g2_font_luBS14_tf);      
      for(int i = 0; i <= count; ++i){
        int str_len = myLine[i].length() + 1;
        char char_array[str_len];
        myLine[i].toCharArray(char_array, str_len);        
        u8g2.drawStr(0, ((i+1)*16)-2, char_array); 
      }
      u8g2.sendBuffer();          

      free(orignal); // Prevents heap memory leaks
    }   
  }
}