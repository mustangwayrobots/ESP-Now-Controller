// ==============================================================================
// Menu.ino - Menu Navigation, Parameter Adjustments, and Calibration
// ==============================================================================
static uint32_t menuDebounceTime = 0;
static uint32_t selectDebounceTime = 0;
static uint32_t navDebounceTime = 0;
uint32_t debounceDelay = 200;
static boolean MenuActive = false;
static int CurrentMenuItem = 0;
static boolean SelectActive = false;

//**********************************************************************************************
// process the menu actions
boolean ActiveMenu(){
  if (MenuButton == true && (millis() - menuDebounceTime) > debounceDelay){
    menuDebounceTime = millis();
    if(MenuActive == false){
      MenuActive = true;
      CurrentMenuItem = 0;
      SelectActive = false;
      displayMsg(Menu[CurrentMenuItem].label);
    }else{
      MenuActive = false;
      SelectActive = false;
      displayMsg("WAITING");            
    }
  }  

  if(MenuActive == true){
    if (SelectButton == true && (millis() - selectDebounceTime) > debounceDelay){      
      selectDebounceTime = millis();
      SelectButtonPressed();
    }
    
    if(SelectActive == true){      
      AdjustParameterValues();  
    }else{  
      CycleMenuItems();           
    }
  }
  return MenuActive;
}

//**********************************************************************************************
//**********************************************************************************************
void SelectButtonPressed(){
  selectDebounceTime = millis();
  if(SelectActive == false){
    SelectActive = true;
    for (int i = 0; i < MenuItems; i++){
      tempSettings[i] = currentSettings[i];
      
      // Capture live raw ADC offset (-512 to 512) around the 2048 center point
      if (strcmp(Menu[CurrentMenuItem].label, "JOYSTICK ZERO-X") == 0 ) {
        tempSettings[CurrentMenuItem] = constrain(analogRead(JOYSTICK_VERT_PIN) - 2048, -512, 512);                    
      }
      if (strcmp(Menu[CurrentMenuItem].label, "JOYSTICK ZERO-Y") == 0 ) {
        tempSettings[CurrentMenuItem] = constrain(analogRead(JOYSTICK_HORZ_PIN) - 2048, -512, 512);                    
      }
    }
    DisplayParameterValue();
  }else{
    SelectActive = false;     
    
    preferences.begin("rc-config", false);
    for (int i = 0; i < MenuItems; i++){
      if (strcmp(Menu[CurrentMenuItem].label, "DEFAULTS") == 0 ){
        tempSettings[i] = defaultSettings[i];
      }
      currentSettings[i] = tempSettings[i];
      
      char key[10];
      snprintf(key, sizeof(key), "set_%d", i);
      preferences.putInt(key, currentSettings[i]);
    }
    preferences.end();
    Serial.println("Settings saved to NVS successfully.");
    
    // If DEFAULTS was triggered, show explicit visual feedback and exit menu mode
    if (strcmp(Menu[CurrentMenuItem].label, "DEFAULTS") == 0 ){
      displayMsg("RESET OK!");
      delay(1000); // Hold message for 1 second so you can read it
      MenuActive = false; // Exit menu back to normal operating mode
      displayMsg("WAITING");
      return;
    }
    
    if (strcmp(Menu[CurrentMenuItem].label, "TEST TRIM") == 0){      
      Serial.println("running A test");
      runtest(); 
    }
    displayMsg(Menu[CurrentMenuItem].label);
  }  
}

//**********************************************************************************************
void AdjustParameterValues(){
  int stepSize = 1;
  if (strcmp(Menu[CurrentMenuItem].label, "JOYSTICK ZERO-X") == 0 || 
      strcmp(Menu[CurrentMenuItem].label, "JOYSTICK ZERO-Y") == 0) {
    stepSize = 10; // Fast scroll for offset values
  }

  if (UpButton == true && (millis() - navDebounceTime) > 150){
    navDebounceTime = millis();
    tempSettings[CurrentMenuItem] = constrain(tempSettings[CurrentMenuItem] + stepSize, Menu[CurrentMenuItem].min, Menu[CurrentMenuItem].max);
    DisplayParameterValue();
  }
  if (DownButton == true && (millis() - navDebounceTime) > 150){
    navDebounceTime = millis();
    tempSettings[CurrentMenuItem] = constrain(tempSettings[CurrentMenuItem] - stepSize, Menu[CurrentMenuItem].min, Menu[CurrentMenuItem].max);
    DisplayParameterValue();
  }
}
//**********************************************************************************************
void DisplayParameterValue(){
  char buf2[50];
  if(strcmp(Menu[CurrentMenuItem].label, "TEST TRIM") == 0 ){
    sprintf (buf2, "%2i sec", tempSettings[2]);        
    displayMsg(buf2);  
  }else if(strcmp(Menu[CurrentMenuItem].label, "MOTOR \"A\" DIRECTION") == 0 ||
           strcmp(Menu[CurrentMenuItem].label, "MOTOR \"B\" DIRECTION") == 0 ||
           strcmp(Menu[CurrentMenuItem].label, "MOTOR A-B RIGHT-LEFT")  == 0 ){
    if(tempSettings[CurrentMenuItem] == 0) displayMsg("NORMAL");
    if(tempSettings[CurrentMenuItem] == 1) displayMsg("REVERSE");
  }else if(strcmp(Menu[CurrentMenuItem].label, "DEFAULTS") == 0 ){
    displayMsg("CONFIRM");      
  }else if(strcmp(Menu[CurrentMenuItem].label, "POWER ON TUNE") == 0 ){    
    char* copy = strdup(songs2[tempSettings[CurrentMenuItem]]);   
    displayMsg(strtok(copy,":"));   
    free(copy);   
  }else{ 
    sprintf (buf2, "%5i", tempSettings[CurrentMenuItem]);
    displayMsg(buf2);      
  }    
}

//**********************************************************************************************
void CycleMenuItems(){
  if (UpButton == true && (millis() - navDebounceTime) > 150){
    navDebounceTime = millis();  
    CurrentMenuItem = (CurrentMenuItem + 1) % MenuItems;
    displayMsg(Menu[CurrentMenuItem].label);   
  }
  if (DownButton == true && (millis() - navDebounceTime) > 150){
    navDebounceTime = millis();  
    CurrentMenuItem = (CurrentMenuItem + MenuItems - 1) % MenuItems;
    displayMsg(Menu[CurrentMenuItem].label);   
  }
}

//**********************************************************************************************
void runtest(){
  char buf2[10];
  uint32_t runtime = millis() + (currentSettings[2]*1000);
  while (millis() < runtime){
    CalculateMotorDriverValues(currentSettings[10], 1023);  
    sprintf (buf2, "%4.1f Secs", (runtime-millis())/1000.0);
    displayMsg(buf2);
    delay(20);
    yield();
  }
  runtime = millis() + 1000;  
  while (millis() < runtime){
    CalculateMotorDriverValues(currentSettings[10], currentSettings[11]);  
    sprintf (buf2, "%4.1f Secs", (runtime-millis())/1000.0);
    displayMsg(buf2);
    delay(20);
    yield();
  }
  runtime = millis() + (currentSettings[2]*1000);  
  while (millis() < runtime){
    CalculateMotorDriverValues(currentSettings[10], -1023);  
    sprintf (buf2, "%4.1f Secs", (runtime-millis())/1000.0);
    displayMsg(buf2);
    delay(20);
    yield();
  }
  Serial.println("test done");
  CalculateMotorDriverValues(currentSettings[10], currentSettings[11]);  
}