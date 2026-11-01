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
      displayMsg(Menu[CurrentMenuItem].label, 255, 1);
    }else{
      MenuActive = false;
      SelectActive = false;
      displayMsg("WAITING", 40, 2);            
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
void SelectButtonPressed(){
  selectDebounceTime = millis();
  if(SelectActive == false){
    SelectActive = true;
    for (int i = 0; i < MenuItems; i++){
      tempSettings[i] = currentSettings[i];
      
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
    
    if (strcmp(Menu[CurrentMenuItem].label, "DEFAULTS") == 0 ){
      displayMsg("RESET OK!", 255, 1);
      delay(1000); 
      MenuActive = false; 
      displayMsg("WAITING", 40, 2);
      return;
    }
    
    if (strcmp(Menu[CurrentMenuItem].label, "TEST TRIM") == 0){      
      Serial.println("running A test");
      runtest(); 
    }
    displayMsg(Menu[CurrentMenuItem].label, 255, 1);
  }  
}

//**********************************************************************************************
void AdjustParameterValues(){
  int stepSize = 1;
  if (strcmp(Menu[CurrentMenuItem].label, "JOYSTICK ZERO-X") == 0 || 
      strcmp(Menu[CurrentMenuItem].label, "JOYSTICK ZERO-Y") == 0) {
    stepSize = 10; 
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
    sprintf (buf2, "%2i sec", tempSettings[CurrentMenuItem]);        
    displayMsg(buf2, 255, 1);  
  }else if(strcmp(Menu[CurrentMenuItem].label, "STEER RATE") == 0 ||
           strcmp(Menu[CurrentMenuItem].label, "STEER EXPO") == 0 ){
    sprintf (buf2, "%3i%%", tempSettings[CurrentMenuItem]); // Formats as percentage e.g. " 40%"
    displayMsg(buf2, 255, 1);
  }else if(strcmp(Menu[CurrentMenuItem].label, "MOTOR \"A\" DIRECTION") == 0 ||
           strcmp(Menu[CurrentMenuItem].label, "MOTOR \"B\" DIRECTION") == 0 ||
           strcmp(Menu[CurrentMenuItem].label, "MOTOR A-B RIGHT-LEFT")  == 0 ){
    if(tempSettings[CurrentMenuItem] == 0) displayMsg("NORMAL", 255, 1);
    if(tempSettings[CurrentMenuItem] == 1) displayMsg("REVERSE", 255, 1);
  }else if(strcmp(Menu[CurrentMenuItem].label, "DEFAULTS") == 0 ){
    displayMsg("CONFIRM", 255, 1);      
  }else if(strcmp(Menu[CurrentMenuItem].label, "POWER ON TUNE") == 0 ){    
    char* copy = strdup(songs2[tempSettings[CurrentMenuItem]]);   
    displayMsg(strtok(copy,":"), 255, 1);   
    free(copy);   
  }else{ 
    sprintf (buf2, "%5i", tempSettings[CurrentMenuItem]);
    displayMsg(buf2, 255, 1);      
  }    
}

//**********************************************************************************************
void CycleMenuItems(){
  if (UpButton == true && (millis() - navDebounceTime) > 150){
    navDebounceTime = millis();  
    CurrentMenuItem = (CurrentMenuItem + 1) % MenuItems;
    displayMsg(Menu[CurrentMenuItem].label, 255, 1);   
  }
  if (DownButton == true && (millis() - navDebounceTime) > 150){
    navDebounceTime = millis();  
    CurrentMenuItem = (CurrentMenuItem + MenuItems - 1) % MenuItems;
    displayMsg(Menu[CurrentMenuItem].label, 255, 1);   
  }
}

//**********************************************************************************************
//**********************************************************************************************
void runtest(){
  char buf2[10];
  
  // 1. FORWARD TEST FIRST (Using -1023 since your normal forward throttle maps to negative rawY)
  uint32_t runtime = millis() + (currentSettings[2]*1000);
  while (millis() < runtime){
    CalculateMotorDriverValues(currentSettings[10], -1023);  
    sprintf (buf2, "%4.1f Secs", (runtime-millis())/1000.0);
    displayMsg(buf2, 255, 1);
    delay(20);
    yield();
  }
  
  // PAUSE / NEUTRAL (1 second center stick)
  runtime = millis() + 1000;  
  while (millis() < runtime){
    CalculateMotorDriverValues(currentSettings[10], currentSettings[11]);  
    sprintf (buf2, "%4.1f Secs", (runtime-millis())/1000.0);
    displayMsg(buf2, 255, 1);
    delay(20);
    yield();
  }
  
  // 2. REVERSE TEST SECOND (Using +1023 for reverse throttle)
  runtime = millis() + (currentSettings[2]*1000);  
  while (millis() < runtime){
    CalculateMotorDriverValues(currentSettings[10], 1023);  
    sprintf (buf2, "%4.1f Secs", (runtime-millis())/1000.0);
    displayMsg(buf2, 255, 1);
    delay(20);
    yield();
  }
  
  Serial.println("test done");
  CalculateMotorDriverValues(currentSettings[10], currentSettings[11]);  
}