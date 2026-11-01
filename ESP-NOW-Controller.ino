/**
 * ==============================================================================
 * ESP-NOW RC Transmitter Core (ESP32-C3 Architecture Layout)
 * ==============================================================================
 */
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <Preferences.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "songs2.h"

Preferences preferences;

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

#define WIFI_CHANNEL 1

// Correct GPIO Pin Definitions (Avoiding bootstrap pins 2 & 9)
#define JOYSTICK_HORZ_PIN 3  // Horz joystick potentiometer on GPIO 3
#define JOYSTICK_VERT_PIN 4  // Vert joystick potentiometer on GPIO 4
#define MENU_BUTTON_PIN   10 // Menu/Bind button on GPIO 10
#define SELECT_BUTTON_PIN 8  // Select button on GPIO 8
#define SEL_BUTTON_PIN    7  // Sel button on GPIO 7
#define UP_BUTTON_PIN     1  // Up button on GPIO 1
#define DOWN_BUTTON_PIN   0  // Down button on GPIO 0
#define PIEZO_PIN         20 // Buzzer MOSFET gate on GPIO 20

boolean NormalMode = true;
boolean TXConnectionStatus = false;
boolean RXConnectionStatus = false;
boolean MenuButton, SelectButton, UpButton, DownButton, SpinButton;
boolean NewDataFlag = false;

// FIXED: volatile keyword ensures main loop instantly reads flag changes from the ESP-NOW callback
volatile boolean SendBindingComplete = false; 
volatile boolean ReceiveBindingComplete = false;

uint8_t BindingMac[] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x99};
uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
int Speed_A = 0;
int Speed_B = 0;
int joystickX;
int joystickY;
boolean MenuMode;

struct __attribute__((packed)) RC_Packet_Structure {
  uint8_t protocolMode; // 0 = Bind Request, 1 = Normal Drive, 2 = Bind Acknowledgement
  int motor_A1; int motor_A2;
  int motor_B1; int motor_B2;
  int weapon;   int battery;
};

RC_Packet_Structure SendingData;
RC_Packet_Structure ReceivingData;

const int MenuItems = 16;

struct menuStruct {
  char label[21];
  int min;
  int max;
} Menu[] = {
  {"MAX SPEED", 1, 128},          // Index 0
  {"TRIM", -127, 127},            // Index 1
  {"TEST TRIM", 1, 10},           // Index 2
  {"STEER RATE", 10, 100},        // Index 3
  {"STEER EXPO", 0, 100},         // Index 4
  {"MOTOR \"A\" DIRECTION", 0, 1},// Index 5
  {"MOTOR \"B\" DIRECTION", 0, 1},// Index 6
  {"MOTOR A-B RIGHT-LEFT", 0, 1}, // Index 7
  {"SPIN SPEED", 0, 127},         // Index 8
  {"WEAPON ON VALUE", 0, 180},    // Index 9
  {"WEAPON OFF VALUE", 0, 180},   // Index 10
  {"JOYSTICK ZERO-X", -512, 512}, // Index 11
  {"JOYSTICK ZERO-Y", -512, 512}, // Index 12
  {"JOYSTICK DEAD ZONE", 0, 127}, // Index 13
  {"POWER ON TUNE", 0, 94},       // Index 14
  {"DEFAULTS", 0, 0}              // Index 15
};

// Exactly 16 default values corresponding to indices 0 through 15
int defaultSettings[MenuItems] = {128, 0, 3, 40, 60, 0, 0, 0, 128, 80, 0, 0, 0, 20, 0, 0};
int tempSettings[MenuItems];
int currentSettings[MenuItems];

// Function Declarations
void setupNormalMode();   
void setupBindingMode();
void loopNormalMode();    
void loopBindingMode();
void readInputs();
void displayMsg(const char *NewMessage, int contrast = 255, int blinkMode = 1);
void CalculateMotorDriverValues(int rawX, int rawY);
void SendData();
boolean ActiveMenu();
void play_rtttl(int TonePin, const char *p);
void Beep(int BeepFrequency, int BeepPeriod, int BeepDuration);

void setup() {
  setCpuFrequencyMhz(80); // Drops clock to 80MHz, saving considerable active power
  Serial.begin(115200); 
  Serial.println("");
  
  Wire.begin(5, 6); // SDA on GPIO 5, SCL on GPIO 6
  u8g2.begin();
  u8g2.setFlipMode(1); // Flips display 180 degrees to fix upside-down text

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIEZO_PIN, OUTPUT);
  
  // Configure input buttons with internal pullups
  pinMode(MENU_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SELECT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SEL_BUTTON_PIN, INPUT_PULLUP);
  pinMode(UP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DOWN_BUTTON_PIN, INPUT_PULLUP);

  // --- Load Settings and MAC from ESP32 NVS (Preferences) ---
  preferences.begin("rc-config", false);
  
  if (preferences.isKey("mac")) {
    String savedMac = preferences.getString("mac", "");
    sscanf(savedMac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &BindingMac[0], &BindingMac[1], &BindingMac[2],
           &BindingMac[3], &BindingMac[4], &BindingMac[5]);
    Serial.println("Loaded bound MAC from NVS.");
  }

  for(int i = 0; i < MenuItems; i++) {
    char key[10];
    snprintf(key, sizeof(key), "set_%d", i);
    currentSettings[i] = preferences.getInt(key, defaultSettings[i]);
  }
  preferences.end();
  // ---------------------------------------------------------
  
  readInputs();    
  if (MenuButton == false){
    NormalMode = true;
    
    // Play power-on tune ONLY when starting in normal mode (Index 15 with shift)
    int tuneIndex = currentSettings[14];
    if (tuneIndex > 0 && tuneIndex < 94) {
      play_rtttl(PIEZO_PIN, songs2[tuneIndex]);
    }

    setupNormalMode();      
  }else{
    NormalMode = false;
    setupBindingMode();  
  }  
}

void loop() {
  yield(); 
  if(NormalMode == true){
    loopNormalMode();
  }else{
    loopBindingMode();
  }
}