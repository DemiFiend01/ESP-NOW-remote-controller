#include <esp_now.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "FS.h"

#define FORMAT_LITTLEFS_IF_FAILED true

// enum value containing the operation state of the platform
enum manager_state {
  STANDBY = 0,
  MOVING = 1, 
  SCANNING = 2,
  UPLOADING = 3 //data to the PC
}

// Must match the sender structure
typedef struct struct_message {
  int x, y;
  bool start, select, x_button, y_button, b_button, a_button;
  op_mode state;
} struct_message;

struct_message rx_message;

//for encryption purposes
uint8_t masterMacAddress[6];

// encryption codes
// It can be made of numbers and letters and the keys are 16 bytes
static const char* PMK_KEY_STR; //sender's
static const char* LMK_KEY_STR; //local relationship between rx and tx

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
    } else if(line.startsWith("LMK:"){
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
  if (sscanf(macString.c_str(), "%x:%x:%x:%x:%x:%x",
             &values[0], &values[1], &values[2],
             &values[3], &values[4], &values[5]) == 6) {
    for (int i = 0; i < 6; ++i) masterMacAddress[i] = (uint8_t) values[i]; //assigning the MAC address of the rx
  } else {
    Serial.println("Invalid MAC format!");
    return;
  }

  PMK_KEY_STR = pmkKey;
  LMK_KEY_STR = lmkKey;
}

// callback function that will be executed when data is received
// modify this however you need
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpye rx_message, incomingData, len);

  Serial.print("X: ");
  Serial.printle rx_message.x);
  Serial.print("Y: ");
  Serial.printle rx_message.y);
  Serial.print("Start: ");
  Serial.printle rx_message.start);
  Serial.print("Select: ");
  Serial.printle rx_message.select);
  Serial.print("X button: ");
  Serial.printle rx_message.x_button);
  Serial.print("Y button: ");
  Serial.printle rx_message.y_button);
  Serial.print("A button: ");
  Serial.printle rx_message.a_button);
  Serial.print("B button: ");
  Serial.printle rx_message.b_button);
  Serial.print("State: ");
  Serial.printle rx_message.state);
}
 
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  //read the keys and MAC
  readFile(LittleFS,"/keys.txt");

  // set the PMK key for encryption
  esp_now_set_pmk((uint8_t *)PMK_KEY_STR);

  // Register the master as peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, masterMacAddress, 6);
  peerInfo.channel = 0;
  // Setting the master device LMK key aka the local key of the relationship
  for (uint8_t i = 0; i < 16; i++) {
    peerInfo.lmk[i] = LMK_KEY_STR[i];
  }
  peerInfo.encrypt = true;
  
  // Add master as peer       
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  // registering the callback function to OnDataRecv
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  // the setup went well
  Serial.println("The receiver board has been initiated.");
}
 
void loop() {
  // does not require code
}

   