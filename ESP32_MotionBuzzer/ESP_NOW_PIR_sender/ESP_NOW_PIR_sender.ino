// Anne de Jong, Feb 2022, ESP-NOW PIR buzzer alarm: SENDER
// see ESP_NOW_receiver_Buzzer for the other part
// ESP always in sleep mode to save battery
// A counter display is used to count the number of triggers


#include <esp_now.h>
#include <WiFi.h>
#include <TM1637Display.h>

// ===================================== PIN CONNECTIONS ==============================================

const int led = 26;
const byte sensorPin = 27;

// TM1637; VCC,  GND, CLK ,    DIO
// ESP32;  5V,   GND, GPIO18,  GPIO19 
const int CLK = 18;        
const int DIO = 19;         
TM1637Display display(CLK, DIO); //set up the 4-Digit Display.


// ===================================== PARAMETERS ==============================================

// choose between Multi or Unibroadcast 
uint8_t broadcastAddress[] = {0xFF, 0xFF,0xFF,0xFF,0xFF,0xFF}; 
// uint8_t broadcastAddress[] = {0x94, 0xB9,0x7E,0xE5,0x1F,0x4C};

RTC_DATA_ATTR int bootCount = 0;   // RTC_DATA_ATTR is used to store data during sleep mode
 
SemaphoreHandle_t syncSemaphore;

// ===================================== FUNCTIONS ==============================================

 
void IRAM_ATTR handleInterrupt() {
  xSemaphoreGiveFromISR(syncSemaphore, NULL);
}
 
void setup() {
  Serial.begin(9600);

  // ========== TM1637 display 
  display.setBrightness(0x0a); //set the diplay to maximum brightness
  
  // ========== ESPNOW ==========
  WiFi.mode(WIFI_STA);
  // MAC of Sender
  Serial.println("WiFi mmacAddress: " + String(WiFi.macAddress()));
 // Init esp_now
  if (esp_now_init() != ESP_OK) {  Serial.println("Error initializing ESP-NOW");   return;  }
  // register esp_now peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  // Add peer to list using esp_now_add_peer function
  if (esp_now_add_peer(&peerInfo) != ESP_OK){  Serial.println("Failed to add peer");  return;  }
  // Test connection by sending int x = 10;
  int x = 10;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &x, sizeof(int));
  if (result == ESP_OK) {  Serial.println("Sent with success");  }
  else {                   Serial.println("Error sending the data");  }

  // ========== use sleep mode to save energy, wake on PIR signal 
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_27,0);  
  
  // ========== PIR and led
  syncSemaphore = xSemaphoreCreateBinary();
  pinMode(led, OUTPUT);
  pinMode(sensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(sensorPin), handleInterrupt, CHANGE);
}
 
void loop() {
    xSemaphoreTake(syncSemaphore, portMAX_DELAY);
    if(digitalRead(sensorPin)){
      Serial.println("Motion detected");
      digitalWrite(led, HIGH);
      ++bootCount; //Increment boot number and print it every reboot when wake-up from sleep mode
      Serial.println("Boot number: " + String(bootCount));
      display.showNumberDec(bootCount);  // show on TM1637 display
      // Test connection by sending int x = 10;
      int x = 10;
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &x, sizeof(int));
      if (result == ESP_OK) {
        Serial.println("Sent with success");
      } else {
          Serial.println("Error sending the data");
      }
    } else {
      digitalWrite(led, LOW);
      Serial.println("Going to sleep now");
      delay(2000);
      esp_deep_sleep_start();
    }
 }
 
