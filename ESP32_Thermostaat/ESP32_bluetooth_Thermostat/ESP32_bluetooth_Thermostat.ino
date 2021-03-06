
// Bluetooth Thermostat using the ESP32 WROOM32
// use the app "Serial Bluetooth Terminal" to control the thermostat; send the text 'help' for options
// October 2021, Anne de Jong

#include <OneWire.h>
#include <DallasTemperature.h>
#include <BluetoothSerial.h>
#include <TM1637Display.h>


int TEMP_THRESHOLD_LOWER = 19; // upper threshold of temperature
int TEMP_THRESHOLD_UPPER = 20; // lower threshold of temperature
int TEMP_SAVE = 3 ;  // Overheat protection; TEMP_THRESHOLD_UPPER + TEMP_SAVE

; // UPPER value for the second relay to prevent overheating is relay 1 would fail
String HEATER_STATUS = "OFF";

int LOG_INTERVAL = 60 ;// log sample time in seconds
unsigned long LastLogMillis ; 
unsigned long LastShutdown ;
const int LOG_COUNT_MAX = 20000  ;
float logValues[LOG_COUNT_MAX] ; 
int logCount = 0 ;

// PIN CONNECTIONS
const int CLK = 18;         // Set the CLK pin connection to the display
const int DIO = 19;         // Se  t the DIO pin connection to the display
const int RELAY_PIN = 27;   // ESP32 pin connected to relay KY-019 of EZ-Delivery
const int oneWireBus = 21;  // GPIO 21 pin is used to read the data from the DS18B20 (yellow wire)    


// Digital thermometer
OneWire oneWire(oneWireBus); // Setup a oneWire instance to communicate with any OneWire DS18B20 
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature sensor 

// WROOM32 Bluetooth
BluetoothSerial SerialBT;

// 4 x 7 Digit display
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
  SerialBT.begin("ESP32thermostat"); //Bluetooth device name
  display.setBrightness(1); //set the TM1637 diplay to maximum brightness = 0x0a
  sensors.begin(); // Start the DS18B20 sensor
  pinMode(RELAY_PIN, OUTPUT);

  Serial.print("Heap size ");
  Serial.println(ESP.getFreeHeap());

  LastLogMillis = millis();  //initial start time
  LastShutdown  = millis();  //initial start time
}

void myprint(String str) {
  SerialBT.println(str); 
  Serial.println(str);
  
}

void help() {
  myprint("[U23 or u23], set UPPER limit to 23??C");
  myprint("[L20 or l20], set LOWER limit to 20??C");
  myprint("[S4 or s4], set SAVE limit to UPPER + 2??C");
  myprint("[I60 or i60 or logI60 or logi60], set log interval to 60 seconds");
  myprint("[log], show all logged values");
  myprint("[logReset or logreset], restart logging data");
  myprint("[limits], show current limits and relay status");
  myprint("[status], show current limits and relay status");
  myprint("[relay], show current limits and relay status");
  myprint("[reset], reset upper and lower limit");
  myprint("[help], print this help");
  myprint("All input is not case sensitive");
  myprint("---- Script written by Anne de Jong ----");
}

void Limits() {
  myprint("Current LOWER limit = "+String(TEMP_THRESHOLD_LOWER)+ "??C");
  myprint("Current UPPER limit = "+String(TEMP_THRESHOLD_UPPER)+ "??C");
  if (HEATER_STATUS == "ON") {
     myprint("The heater is turned ON"); }
  else {myprint("The heater is turned OFF");}   
}

void ResetLimits() {
  TEMP_THRESHOLD_UPPER = 21;
  TEMP_THRESHOLD_LOWER = 19;
  myprint("Reset LOWER limit to "+String(TEMP_THRESHOLD_LOWER)+ "??C");
  myprint("Reset UPPER limit to "+String(TEMP_THRESHOLD_UPPER)+ "??C");
  
}

void SetNewValues(String Query) {
    if(Query.indexOf("U") == 0 ) {   // e.g., u33
      Query.remove(0,1);
      int QueryVal = Query.toInt(); 
      if (QueryVal!=0 && QueryVal>TEMP_THRESHOLD_LOWER) {
          myprint("New UPPER limit = "+String(QueryVal)+ "??C");
          TEMP_THRESHOLD_UPPER = QueryVal;
        } else { 
          myprint("ERROR: UPPER should be higher than LOWER limit") ; 
        }
    } 
    if(Query.indexOf("L") == 0 ) {   // e.g., L28  
      Query.remove(0,1);
      int QueryVal = Query.toInt(); 
      if (QueryVal!=0 && QueryVal<TEMP_THRESHOLD_UPPER) {
        myprint("New LOWER limit = "+String(QueryVal)+ "??C");
        TEMP_THRESHOLD_LOWER = QueryVal;
        } else { 
          myprint("ERROR: UPPER should be higher than LOWER limit") ; 
        } 
     }
     if(Query.indexOf("I") == 0 ) {   // e.g., i60  set log interval  
      Query.remove(0,1);
      int QueryVal = Query.toInt(); 
      if (QueryVal>5 && QueryVal<3601) {
        myprint("New Log Interval = "+String(QueryVal)+ " seconds");
        LOG_INTERVAL = QueryVal;
        } else { 
          myprint("ERROR: Log interval should be between 6 and 3600 seconds") ; 
        } 
     }
    
}


void logTemperature(float temperature) {  // add value to the log array
  if (millis() - LastLogMillis > 1000*LOG_INTERVAL) {
    myprint("Add to log; "+ String(logCount) + " ; t=" + String(logCount*LOG_INTERVAL) + " ; " + String(temperature) +"??C");
    logValues[logCount] = temperature ;
    logCount += 1 ;
    LastLogMillis = millis();
  }
}

void logShow() {  // show the log
  myprint("ID,Seconds,Minutes,Hours,Temp/??C");
  for (int i=0; i<logCount; i++) {
    myprint(String(i) + "\t" +String(i*LOG_INTERVAL)+ "\t" +String(i*LOG_INTERVAL/60)+ "\t" +String(i*LOG_INTERVAL/3600) + "\t" + String(logValues[i]));
  }  
}


void loop() {
  // read the temperature
  sensors.requestTemperatures(); 
  float temperature = sensors.getTempCByIndex(0);
  myprint(String(temperature)+"??C");

  logTemperature(temperature) ;


  // Tijdelijke oplossing reset heater: 1x per 3600 seconds, 5 seconds off
  if (millis() - LastShutdown > 1000*3600) {
    myprint("RESET: Power OFF for 5 seconds");
    digitalWrite(RELAY_PIN, LOW); // turn OFF
    HEATER_STATUS == "OFF" ;
    LastShutdown = millis();
    delay(5000) ;
  }
  
  // send temperature data via Bluetooth
  if (SerialBT.available()) {  
    // Read new limits from BT; U or u for Upper L or l for Lower; e.g.,  U33 or L21
    String BTquery=SerialBT.readString();
    BTquery.toUpperCase();
    BTquery.trim();
    if (BTquery == "RESET" ) { ResetLimits(); }
    else if (BTquery == "HELP" ) { help(); }
    else if (BTquery == "LOG" ) { logShow(); }
    else if (BTquery == "LOGRESET" ) { logCount=0; }
    else if (BTquery == "LIMITS" | BTquery == "STATUS" | BTquery == "RELAY" ) { Limits(); }
    else { SetNewValues(BTquery); }  // Upper, Lower and log Inteval
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
  if(temperature > TEMP_THRESHOLD_UPPER + TEMP_SAVE){
    myprint("WARNING, Overheating detected; shutting down relay 2");
  }
  delay(3000);


  // Show info on the TMP1637 display
  display.showNumberDec(TEMP_THRESHOLD_LOWER, false, 2, 2);  // number, leading_zeros, length, position
  display.setSegments(LOWER, 2, 0);
  delay(1500);
  display.showNumberDec(TEMP_THRESHOLD_UPPER, false, 2, 2);  // number, leading_zeros, length, position
  display.setSegments(UPPER, 2, 0);
  delay(1500);
  display.showNumberDec(round(temperature), false, 2, 0);
  display.setSegments(celsius, 2, 2);
  delay(3000);
  
}
