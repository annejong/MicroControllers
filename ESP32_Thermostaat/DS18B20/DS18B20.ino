
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BluetoothSerial.h>
#include <TM1637Display.h>

int TEMP_THRESHOLD_UPPER = 19; // upper threshold of temperature, change to your desire value
int TEMP_THRESHOLD_LOWER = 21; // lower threshold of temperature, change to your desire value
String HEATER_STATUS = "OFF";

const int CLK = 18;         // Set the CLK pin connection to the display
const int DIO = 19;         // Set the DIO pin connection to the display
const int RELAY_PIN = 27;   // ESP32 pin connected to relay KY-019 of EZ-Delivery
const int oneWireBus = 21;  // GPIO 21 pin is used to read the data from the DS18B20 (yellow wire)    


OneWire oneWire(oneWireBus); // Setup a oneWire instance to communicate with any OneWire DS18B20 
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature sensor 

BluetoothSerial SerialBT;

// https://www.makerguides.com/tm1637-arduino-tutorial/
TM1637Display display(CLK, DIO); //set up the 4-Digit Display.
// Create degree Celsius symbol:
const uint8_t celsius[] = {
  SEG_A | SEG_B | SEG_F | SEG_G,  // Circle
  SEG_A | SEG_D | SEG_E | SEG_F   // C
};

const uint8_t HEAT[] = {
  SEG_B | SEG_C | SEG_E | SEG_F | SEG_G, // H
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G, // E
  SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G, // A
  SEG_D | SEG_E | SEG_F | SEG_G   // T
};

const uint8_t OFF[] = {
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F, // O
  SEG_A | SEG_E | SEG_F | SEG_G, // F
  SEG_A | SEG_E | SEG_F | SEG_G // F
};

const uint8_t UPPER[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_F, // U
  SEG_D | SEG_G // =
};

const uint8_t LOWER[] = {
  SEG_D | SEG_E | SEG_F, // L
  SEG_D | SEG_G // =
};


void setup() {
  Serial.begin(9600); // Start the Serial Monitor
  SerialBT.begin("ESP32klimaatkamer"); //Bluetooth device name
  display.setBrightness(1); //set the TM1637 diplay to maximum brightness = 0x0a
  sensors.begin(); // Start the DS18B20 sensor
  pinMode(RELAY_PIN, OUTPUT);
}

void myprint(String str) {
  SerialBT.println(str); 
  Serial.println(str);
  
}

void help() {
  myprint("[u23], set upper limit to 23°C");
  myprint("[l20], set lower limit to 20°C");
  myprint("[limits], show current limits and relay status");
  myprint("[status], show current limits and relay status");
  myprint("[relay], show current limits and relay status");
  myprint("[reset], reset upper and lower limit");
  myprint("[help], print this help");
  myprint("input is not case sensitive");
  myprint("Script written by Anne de Jong");
}

void Limits() {
  myprint("Current LOWER limit = "+String(TEMP_THRESHOLD_LOWER)+ "°C");
  myprint("Current UPPER limit = "+String(TEMP_THRESHOLD_UPPER)+ "°C");
  if (HEATER_STATUS == "ON") {
     myprint("The heater is turned ON"); }
  else {myprint("The heater is turned OFF");}   
}

void ResetLimits() {
  TEMP_THRESHOLD_UPPER = 21;
  TEMP_THRESHOLD_LOWER = 19;
  myprint("Reset LOWER limit to "+String(TEMP_THRESHOLD_LOWER)+ "°C");
  myprint("Reset UPPER limit to "+String(TEMP_THRESHOLD_UPPER)+ "°C");
  
}

void SetNewLimits(String NewTemp) {
    NewTemp.toUpperCase();
    Serial.println(NewTemp);
    if(NewTemp.indexOf("U") == 0 ) {   // e.g., u33
      NewTemp.remove(0,1);
      int TempVal = NewTemp.toInt(); 
      if (TempVal!=0 && TempVal>TEMP_THRESHOLD_LOWER) {
          myprint("New UPPER limit = "+String(TempVal)+ "°C");
          TEMP_THRESHOLD_UPPER = TempVal;
        } else { 
          myprint("ERROR: UPPER should be higher than LOWER limit") ; 
        }
    } 
    if(NewTemp.indexOf("L") == 0 ) {   // e.g., L28  
      NewTemp.remove(0,1);
      int TempVal = NewTemp.toInt(); 
      if (TempVal!=0 && TempVal<TEMP_THRESHOLD_UPPER) {
        myprint("New LOWER limit = "+String(TempVal)+ "°C");
        TEMP_THRESHOLD_LOWER = TempVal;
        } else { 
          myprint("ERROR: UPPER should be higher than LOWER limit") ; 
        } 
     }
}



void loop() {
  // read the temperature
  sensors.requestTemperatures(); 
  float temperature = sensors.getTempCByIndex(0);
  myprint(String(temperature)+"°C");

  // send temperature data via Bluetooth
  if (SerialBT.available()) {  
    // Read new limits from BT; U or u for Upper L or l for Lower; e.g.,  U33 or L21
    String BTquery=SerialBT.readString();
    BTquery.toUpperCase();
    BTquery.trim();
    if (BTquery == "RESET" ) { ResetLimits(); }
    else if (BTquery == "HELP" ) { help(); }
    else if (BTquery == "LIMITS" | BTquery == "STATUS" | BTquery == "RELAY" ) { Limits(); }
    else { SetNewLimits(BTquery); }
  }

  // Switch relay ON or OFF
  if(temperature < TEMP_THRESHOLD_LOWER){
    if (HEATER_STATUS == "OFF") {myprint("The heater is turned ON"); }
    digitalWrite(RELAY_PIN, HIGH); // turn on
    display.setSegments(HEAT, 4, 0);
    HEATER_STATUS = "ON" ;
  } else if(temperature > TEMP_THRESHOLD_UPPER){
    if (HEATER_STATUS == "ON") {myprint("The heater is turned OFF"); }
    digitalWrite(RELAY_PIN, LOW); // turn OFF
    display.clear();
    display.setSegments(OFF, 3, 1);
    HEATER_STATUS = "OFF" ;
  } 
  delay(3000);

  // Show info on the TMP1637 display
  display.showNumberDec(round(temperature), false, 2, 0);
  display.setSegments(celsius, 2, 2);
  delay(3000);
  display.showNumberDec(TEMP_THRESHOLD_LOWER, false, 2, 2);  // number, leading_zeros, length, position
  display.setSegments(LOWER, 2, 0);
  delay(2000);
  display.showNumberDec(TEMP_THRESHOLD_UPPER, false, 2, 2);  // number, leading_zeros, length, position
  display.setSegments(UPPER, 2, 0);
  delay(2000);
  
}
