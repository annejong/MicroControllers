/*
  ESP32 Temperature Web Server - STA Mode
  modified by Anne de Jong
*/

#include <WiFi.h>
#include <WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// SSID & Password
const char* ssid = "Hartelijk";  // Enter your SSID here
const char* password = "Welkomja123";  //Enter your Password here
WebServer server(80);  // Object of WebServer(HTTP port, 80 is defult)

// PIN CONNECTIONS
const int oneWireBus = 21;  // GPIO 21 pin is used to read the data from the DS18B20 (yellow wire)    
                            // DS18B20 Board connections: red 3-5V;  black GND;  yellow, GPIO 21 and 4.7kOhm to 3-5V

// Digital thermometer
OneWire oneWire(oneWireBus); // Setup a oneWire instance to communicate with any OneWire DS18B20 
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature sensor 


int LOG_INTERVAL = 60 ;// log sample time in seconds
unsigned long LastLogMillis ; 
const int maxXY=200 ;         // number of measurements in graph
float y[maxXY] ;              // Temperature values 
String HTML = "";
String strYvalues = "";
String strXvalues ;
float maxTemp ;
float minTemp ;

void setup() {
  Serial.begin(9600);
  Serial.println("Try Connecting to ");
  Serial.println(ssid);

  // Connect to your wi-fi modem
  WiFi.begin(ssid, password);

  // Check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
  delay(1000);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected successfully");
  Serial.print("Got IP: ");
  Serial.println(WiFi.localIP());  //Show ESP32 IP on serial

  server.on("/", handle_root);
  server.begin();
  Serial.println("HTTP server started");
  delay(100); 

  // Start the DS18B20 digital temp sensor
  sensors.begin(); 

  LastLogMillis = millis();  //initial start time
  // Make xValues string
  strXvalues = '[' ;
  for (int i = 0; i < maxXY; i++) {  strXvalues += String(i)+","; }  
  strXvalues += String(maxXY) + ']' ; // add the last without comma
  // set all initial temp values to 20°C
  for (int i = 0; i <= maxXY; i++) {  y[i] = 20; }  
  
}


void UpdateHTML(float temperature ) {
  // HTML & CSS contents which display on web server
  HTML = "<html>" ;
  HTML += "<script src=https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.9.4/Chart.js></script>";
  HTML += "<body><head><meta http-equiv=refresh content=15></head><h1>Hondsrugbier ESP32 - Temperatuur Webserver</h1>" ;
  HTML += "<canvas id=myChart style=width:100%;max-width:1000px></canvas>" ;
  HTML += "Current temperature = "+String(temperature)+"&#176;C<br>";
  HTML +="<a href=data:application/octet-stream,"+strYvalues+">Download data</a>";
  // HTML += "<br><br>Temp values<hr><br>"+strYvalues+"<br><hr>" ;
  // Add the chart.js script
  HTML += "\
  <script>                                                     \
    var xValues = "+strXvalues+";      \
    var yValues = "+strYvalues+";                  \
    new Chart(\"myChart\", {                                       \
      type: \"line\",                                              \
      data: {                                                    \
        labels: xValues,                                         \
        datasets: [{                                             \
          fill: false,                                           \
          lineTension: 0,                                        \
          backgroundColor: \"rgba(0,0,255,1.0)\",                  \
          borderColor: \"rgba(0,0,255,0.1)\",                      \
          data: yValues                                          \
        }]                                                       \
      },                                                         \
      options: {                                                 \
        legend: {display: false},                                \
        scales: {                                                \
          xAxes: [{                                              \
            scaleLabel: {display: true,  labelString: 'Time in minutes after sampling'} \
          }],  \
          yAxes: [{                                              \
            ticks: {min: "+String(minTemp)+", max:"+String(maxTemp)+"}, \
            scaleLabel: {display: true,  labelString: 'Temperature in °C'} \
          }]  \
        }                                                        \
      }                                                          \
    });                                                          \
  </script> \
  ";
    
  HTML += "</body></html>";
}

void add_temp(float t) {
  strYvalues = "[";
  for (int i = 0; i < maxXY-1; i++) {
    y[i] = y[i+1] ; 
    strYvalues += String(y[i]) + "," ; 
  }
  y[maxXY-1] = t ;
  strYvalues += String(t) + "]" ; 
}

void add_temp_reverse(float t) {
  y[maxXY-1] = t ;
  strYvalues = "[";
  for (int i = 0; i < maxXY-1; i++) {
    y[i] = y[i+1] ; 
    strYvalues += String(y[maxXY-1-i  ]) + "," ; 
  }
  strYvalues += String(y[0]) + "]" ; 
}





void loop() {
  sensors.requestTemperatures();  // read the temperature
  float temperature = sensors.getTempCByIndex(0);
  if (millis() - LastLogMillis > 1000*LOG_INTERVAL) {
    LastLogMillis = millis();
    add_temp_reverse(temperature) ;
    maxTemp = -273 ;
    minTemp = +273 ;
    for (int i = 0; i < maxXY; i++) { maxTemp = max(maxTemp, y[i]);  minTemp = min(minTemp, y[i]); }   // get temp lmits
    maxTemp = round(maxTemp+2) ;
    minTemp = round(minTemp-2) ;
    UpdateHTML(temperature);
    Serial.println("Temperature value send to web server= "+String(temperature)+"°C  ") ;
  }    
  Serial.println("Current temp = "+String(temperature)+"°C  ") ;
  server.handleClient();
  handle_root();

  delay(3000) ;  // 3 seconds
}



// Handle root url (/)
void handle_root() {
  server.send(200, "text/html", HTML);
}
