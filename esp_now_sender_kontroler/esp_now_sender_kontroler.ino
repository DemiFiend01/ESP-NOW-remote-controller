#include <Wire.h>
#include "Adafruit_seesaw.h"
#include <esp_now.h>
#include <WiFi.h>

#include <LittleFS.h>
#include "FS.h"

#define FORMAT_LITTLEFS_IF_FAILED true

Adafruit_seesaw ss;

//I2C controller button mask
#define BUTTON_X         6
#define BUTTON_Y         2
#define BUTTON_A         5
#define BUTTON_B         1
#define BUTTON_SELECT    0
#define BUTTON_START    16
uint32_t button_mask = (1UL << BUTTON_X) | (1UL << BUTTON_Y) | (1UL << BUTTON_START) |
                       (1UL << BUTTON_A) | (1UL << BUTTON_B) | (1UL << BUTTON_SELECT);

// rx MAC
uint8_t broadcastAddress[6];

// enum value containing the operation state of the platform
enum manager_state {
  STANDBY = 0,
  MOVING = 1, 
  SCANNING = 2,
  UPLOADING = 3 //data to the PC
};

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  int x, y;
  bool start, select, x_button, y_button, b_button, a_button;
  manager_state state;
} struct_message;

// Create a struct_message called myData
struct_message myData;

//for esp-now
esp_now_peer_info_t peerInfo;

// encryption codes
// It can be made of numbers and letters and the keys are 16 bytes
static char PMK_KEY_STR[17]; //sender's
static char LMK_KEY_STR[17]; //local relationship between rx and tx


// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

//for the controller managment
static int last_x = 0, last_y = 0;
static manager_state last_op_mode = STANDBY;

//creates a message to send from reading the input and previous information
void constructMessage(struct_message& new_message)
{
  // Set values to send
  int x = 1023 - ss.analogRead(14);
  int y = 1023 - ss.analogRead(15);
  new_message.a_button = false;
  new_message.b_button = false;
  new_message.x_button = false;
  new_message.y_button = false;
  new_message.select = false;
  new_message.start = false;
  new_message.state = last_op_mode;

  if(new_message.state != SCANNING) //can only move when not scanning
  {
    if ( (abs(x - last_x) > 3)  ||  (abs(y - last_y) > 3)) {
        Serial.print("x: "); Serial.print(x); Serial.print(", "); Serial.print("y: "); Serial.println(y);
        last_x = x;
        last_y = y;
        new_message.x = x;
        new_message.y = y;
        new_message.state = MOVING;
      }
  }

  //read from the mask all the values
  uint32_t buttons = ss.digitalReadBulk(button_mask);

  //Serial.println(buttons, BIN);

  if (! (buttons & (1UL << BUTTON_A))) {
    Serial.println("Button A pressed");
    new_message.a_button = true;
    new_message.state = SCANNING;
  }
  if (! (buttons & (1UL << BUTTON_B))) {
    Serial.println("Button B pressed");
    new_message.b_button = true;
    new_message.state = UPLOADING;
  }
  if (! (buttons & (1UL << BUTTON_Y))) {
    Serial.println("Button Y pressed");
    new_message.y_button = true;
    if(new_message.state == SCANNING || new_message.state == UPLOADING)
    {
      //cancel the scanning or uploading
      new_message.state = STANDBY;
    }
  }

  // i have no idea what to do with those
  if (! (buttons & (1UL << BUTTON_X))) {
    Serial.println("Button X pressed");
    new_message.x_button = true;
  }
  if (! (buttons & (1UL << BUTTON_SELECT))) {
    Serial.println("Button SELECT pressed");
    new_message.select = true;
  }
  if (! (buttons & (1UL << BUTTON_START))) {
    Serial.println("Button START pressed");
    new_message.start = true;
  }

  last_op_mode = new_message.state;

}

void readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  String pmkKey, lmkKey, mac;
  File file = fs.open(path);
  if(!file || file.isDirectory()){
      Serial.println("- failed to open file for reading");
      return;
  }

  while(file.available()){
    Serial.write(file.read());

    String line = file.readStringUntil('\n');
    line.trim();

    if (line.startsWith("PMK:")) {
      pmkKey = line.substring(4);
    } else if(line.startsWith("LMK:")){
      lmkKey = line.substring(4);
    }else if (line.startsWith("RX_MAC:")) {
      mac = line.substring(7);
    }
  
  }
  file.close();
  Serial.println("Keys loaded:");
  Serial.println("PMK: " + pmkKey);
  Serial.println("LMK: " + lmkKey);
  Serial.println("Mac: " + mac);

  //setting the mac address
  int values[6];
  if (sscanf(mac.c_str(), "%x:%x:%x:%x:%x:%x",
             &values[0], &values[1], &values[2],
             &values[3], &values[4], &values[5]) == 6) {
    for (int i = 0; i < 6; ++i) broadcastAddress[i] = (uint8_t) values[i]; //assigning the MAC address of the rx
  } else {
    Serial.println("Invalid MAC format!");
    return;
  }

  pmkKey.toCharArray(PMK_KEY_STRING, sizeof(PMK_KEY_STRING));
  lmkKey.toCharArray(LMK_KEY_STRING, sizeof(LMK_KEY_STRING));
  // PMK_KEY_STR = pmkKey;
  // LMK_KEY_STR = lmkKey;
}

void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  Wire.begin(8,9);
  delay(50);

  //initialise the seesaw with the address
  if(!ss.begin(0x50)){
    Serial.println("ERROR! seesaw not found");
    while(1) delay(1); //freeze the program
  }

  Serial.println("Gamepad QT example!");
  Serial.println("seesaw started");
  //do we have the correct version?
  uint32_t version = ((ss.getVersion() >> 16) & 0xFFFF);
  if (version != 5743) {
    Serial.print("Wrong firmware loaded? ");
    Serial.println(version);
    while(1) delay(10);
  }
  
  Serial.println("Found Product 5743");
  
  //set up the mask for the i2c minicontroller
  ss.pinModeBulk(button_mask, INPUT_PULLUP);
  ss.setGPIOInterrupts(button_mask, 1);

  //read the keys and MAC
  readFile(LittleFS,"/keys.txt");

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  

  //encryption
  //setting the PMK key of the sender
  esp_now_set_pmk((uint8_t *)PMK_KEY_STR);
  //setting the LMK key of the internal local relationship between rx and tx
  for (uint8_t i = 0; i < 16; i++) {
    peerInfo.lmk[i] = LMK_KEY_STR[i];
  }
  peerInfo.encrypt = true;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

}


void loop() {
  
  constructMessage((uint8_t *) &myData);

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(10);
}