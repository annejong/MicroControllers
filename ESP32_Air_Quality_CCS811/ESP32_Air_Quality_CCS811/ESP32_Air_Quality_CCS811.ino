/*
  ccs811basic.ino - Demo sketch printing results of the CCS811 digital gas sensor for monitoring indoor air quality from ams.
  Created by Maarten Pennings 2017 Dec 11
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>    // I2C library
#include "ccs811.h"  // CCS811 library
#include <Adafruit_SSD1306.h>  // OLED SSD1306

// ===================================== PIN CONNECTIONS ==============================================

// PIN CONNECTIONS; CCS811 air sensor with nWAKE to 23 (or GND)
// CCS811; VCC,  GND, SCL,    SDA,    WAK,    INT, RST, ADD
// ESP32 ; 3.3V, GND, GPIO22, GPIO21, GPIO23,  -    -    -
CCS811 ccs811(23); // nWAKE on 23


// PIN CONNECTIONS; SSD1306 OLED display 
// SSD1306; GND,  VCC,   SCL,    SDA
// ESP32  ; GND,  3.3V,  GPIO22, GPIO21
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// ===================================== PARAMETERS ==============================================

// SSID & Password
const char* ssid = "Hartelijk";  // Enter your SSID here
const char* password = "Welkomja123";  //Enter your Password here
WebServer server(80);  // Object of WebServer(HTTP port, 80 is defult)

int LOG_INTERVAL = 6 ;// log sample time in seconds
unsigned long LastLogMillis ; 
const int maxX=1000 ;         // number of measurements in graph
float CO2[maxX] ;             // CO2 values 
String CO2_plotvalues ;            // Hold values as a string for plotting
float VOC[maxX] ;             // CO2 values 
String VOC_plotvalues ;            // Hold values as a string for plotting
String HTML = "";
String X_plotvalues ;
float maxTemp ;
float minTemp ;
float maxCO2 = 32000 ;
float maxVOC = 32000 ;
String IAQ_names[5]  = {"excellent","good","moderate","poor","unhealty"}  ; // Standards for Indoor Air Quality (IAQ)
String IAQ_colors[5] = {"blue","green","yellow","orange","red"}  ; // Standards for Indoor Air Quality (IAQ)
float CO2_IAQ[4] = {400,1000,2000,5000} ;   // excellent < 400 ppm, good< 1000, etc
float VOC_IAQ[4] = {65, 220,660, 2200};     // excellent < 65 ppb, good <220,  etc

// ===================================== FUNCTIONS ==============================================

void setup() {
  // Enable serial
  Serial.begin(9600);

  // Wifi connection
  Serial.println("Try Connecting to "+ String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(1000);  Serial.print(".");  }
  Serial.println("WiFi connected successfully");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP());  //Show ESP32 IP on serial

  // Start Web server
  server.on("/", handle_root);
  server.begin();
  Serial.println("Web server started");
  delay(100); 

  // Enable I2C
  Wire.begin(); 

  // Start CCS811 sensor
  Serial.println("Starting CCS811");
  ccs811.set_i2cdelay(50); // Needed for ESP8266 because it doesn't handle I2C clock stretch correctly
  bool ok= ccs811.begin();
  if( !ok ) Serial.println("setup: CCS811 begin FAILED");

  // Print CCS811 versions
  Serial.print("ccs811 hardware    version: "); Serial.println(ccs811.hardware_version(),HEX);
  Serial.print("ccs811 bootloader  version: "); Serial.println(ccs811.bootloader_version(),HEX);
  Serial.print("ccs811 application version: "); Serial.println(ccs811.application_version(),HEX);
  
  // Start measuring
  ok= ccs811.start(CCS811_MODE_1SEC);
  if( !ok ) Serial.println("setup: CCS811 start FAILED");


  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0,0); display.print("ccs811") ;
  display.setTextSize(1);  
  display.setCursor(0,20); display.print("ccs811 versions") ;
  display.setCursor(0,30); display.print("hardware    : " + String(ccs811.hardware_version(),HEX)); 
  display.setCursor(0,40); display.print("bootloader  : " + String(ccs811.bootloader_version(),HEX)); 
  display.setCursor(0,50); display.print("application : " + String(ccs811.application_version(),HEX)); 
  display.display();
  delay(5000); 

  // Setting Plotvalues
  LastLogMillis = millis();  //initial start time
  // Make xValues string
  X_plotvalues = '[' ;
  for (int i = 0; i < maxX; i++) {  X_plotvalues += String(i)+","; }  
  X_plotvalues += String(maxX) + ']' ; // add the last without comma
  // set all initial CO2=400 and VOC=0
  for (int i = 0; i <= maxX; i++) {  CO2[i] = 400; VOC[i] = 0; }  
  
}

// Handle root url (/)
void handle_root() {
  server.send(200, "text/html", HTML);
}



void UpdateHTML() {
  // HTML & CSS contents which display on web server
  HTML = "<html>" ;
  //HTML += "<script src=https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.9.4/Chart.js></script>";
  HTML += "<script src=https://cdnjs.cloudflare.com/ajax/libs/Chart.js/3.7.0/chart.min.js></script>";
  HTML += "<body><head><meta http-equiv=refresh content=15></head><h1>Hondsrugbier CO2 and VOC (Volatile Organic Compounds) </h1>" ;
  HTML += "<canvas id=myChart style=width:100%;max-width:1000px></canvas>" ;
  HTML +="<a href=data:application/octet-stream,"+CO2_plotvalues+" download='CO2data.csv'>Download CO2 data</a><br>";
  HTML +="<a href=data:application/octet-stream,"+VOC_plotvalues+" download='VOCdata.csv'>Download VOC data</a>";
  // HTML += "<br><br>Temp values<hr><br>"+CO2_plotvalues+"<br><hr>" ;
  // Add the chart.js script
  HTML += " \
  <script>                                                     \
    var xValues = "+X_plotvalues+";      \
    var CO2values = "+CO2_plotvalues+";                  \
    var VOCvalues = "+VOC_plotvalues+";                  \
    new Chart('myChart', {                                                                                        \
      type:'line',                                                                                                \
      data: {                                                                                                     \
        labels: xValues,                                                                                          \
        datasets: [{                                                                                              \
          label: 'CO2',                                                                                           \
      yAxisID: 'CO2',                                                                                         \
          fill: false,                                                                                            \
          lineTension: 0.3,                                                                                       \
          backgroundColor:'rgba(0,128,0,1.0)',                                                                    \
          borderColor:'rgba(0,128,0,0.1)',                                                                        \
          data: CO2values                                                                                         \
        }, {                                                                                                      \
          label: 'VOC',                                                                                           \
      yAxisID: 'VOC',                                                                                         \
          fill: false,                                                                                            \
          lineTension: 0.3,                                                                                       \
          backgroundColor:'rgba(0,0,255,1.0)',                                                                    \
          borderColor:'rgba(0,0,255,0.1)',                                                                        \
          data: VOCvalues                                                                                         \
        }]                                                                                                        \
      },                                                                                                          \
      options: {                                                                                                  \
        legend: {display: false},                                                                                 \
        scales: {                                                                                                 \
      x: {                                                                                                  \
        display: true,                                                                                    \
        title: { display: true, text: 'Time in minutes'}                                          \
      },                                                                                                    \
      CO2: {                                                                                                \
          type: 'linear',                                                                                   \
        display: true,                                                                                    \
        position: 'left',                                                                                 \
        title: { display:true, text: 'ppm CO2' }                                                          \
      },                                                                                                    \
      VOC: {                                                                                                \
          type: 'linear',                                                                                   \
        display: true,                                                                                    \
        position: 'right',                                                                                \
        title: { display:true, text: 'ppb VOC (Volatile Organic Compounds)' },                            \
        grid: { drawOnChartArea: false }                                                                  \
      }                                                                                                     \
        }                                                                                                         \
      }                                                                                                           \
    });                                                                                                           \
  </script>                                                                                                       \
  ";
    
  HTML += "</body></html>";
}


String ArrayToString(float arr[]) {
  String Str = "[";
  for (int i = 0; i < maxX-1; i++) {    Str += String(arr[i]) + "," ;   }
  return Str += "]";
}


void add_CO2(float t) {
  // Shift array and Add CO2 value 
  for (int i = 0; i < maxX-1; i++) {    CO2[i] = CO2[i+1] ;   }
  CO2[maxX-1] = t ;
}

void add_VOC(float t) {
  // Shift array and Add VOC value 
  for (int i = 0; i < maxX-1; i++) {    VOC[i] = VOC[i+1] ;   }
  VOC[maxX-1] = t ;
}



void UpdateCCS811values(uint16_t eco2, uint16_t etvoc) {
  Serial.print("CO2 " + String(eco2)+ " ppm   "); 
  Serial.println("eTVOC "+String(etvoc)+ " ppb");
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0,0);  display.print("CO2 " + String(eco2)) ;
  display.setCursor(0,34); display.print("VOC " + String(etvoc)) ;
  display.setTextSize(1);
  display.setCursor(100,6); display.print("ppm") ;
  display.setCursor(100,40); display.print("ppb") ;
  display.display();
  if (millis() - LastLogMillis > 1000*LOG_INTERVAL) {  // log values to web server
    LastLogMillis = millis();
    Serial.println("Log data");
    add_CO2(eco2) ;
    add_VOC(etvoc) ;
    CO2_plotvalues =  ArrayToString(CO2) ; // update the values for plotting  
    VOC_plotvalues =  ArrayToString(VOC) ; // update the values for plotting  
    maxCO2 = 0 ;   for (int i = 0; i < maxX; i++) { maxCO2 = max(maxCO2, CO2[i]); } 
    maxVOC = 0 ;   for (int i = 0; i < maxX; i++) { maxVOC = max(maxVOC, VOC[i]); } 
    UpdateHTML();
    
    
  }  
}

void loop() {
  // Read
  uint16_t eco2, etvoc, errstat, raw;
  ccs811.read(&eco2,&etvoc,&errstat,&raw); 
  if( errstat==CCS811_ERRSTAT_OK ) { 
    UpdateCCS811values(eco2, etvoc) ;
    
  } else if( errstat==CCS811_ERRSTAT_OK_NODATA ) {
    Serial.println("CCS811: waiting for (new) data");
  } else if( errstat & CCS811_ERRSTAT_I2CFAIL ) { 
    Serial.println("CCS811: I2C error");
  } else {
    Serial.print("CCS811: errstat="); Serial.print(errstat,HEX); 
    Serial.print("="); Serial.println( ccs811.errstat_str(errstat) ); 
  }
  server.handleClient();
  handle_root();
  // Wait
  delay(1000); 
}
