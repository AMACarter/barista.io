///////////////////////////////////////////////
//////////////// BARISTA.IO ///////////////////
/////////////// By Adam Cater /////////////////
/// https://github.com/AMACarter/barista.io ///
///////////////////////////////////////////////

/*  
 *  Using:
 *          IFTTT, Sinric.com, Amazon Alexa
 *  Features: 
 *          
 *          Logs on Google sheets - instant data logging 
 *          Toggle machine using sinric API + amazon Alexa 
 *          Automatic scheduling and brew times
 *          Real-time clock with cleaning cycles
 *          Built-in routines and sugar deliver system
 *
 */

#include <ESP8266WiFi.h> // 
#include <ESP8266WiFiMulti.h>  
#include <WebSocketsClient.h> //  https://github.com/kakopappa/sinric/wiki/How-to-add-dependency-libraries
#include <ArduinoJson.h> // https://github.com/kakopappa/sinric/wiki/How-to-add-dependency-libraries
#include <StreamString.h>
#include <Servo.h>
#include <Time.h> // Time management for self-cleaning cycle
#include <TimeLib.h>


// Uncomment for "WIP features
/*
 * #include <LiquidCrystal.h>
 * #include <WebSerial.h>
 * #include <webserial_webpage.h>
 * 
 * const int RS = D2, EN = D1, d4 = D5, d5 = D6, d6 = D7, d7 = D8;   
 * LiquidCrystal lcd(RS, EN, d4, d5, d6, d7);
 * 
 */

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
WiFiClient client;
Servo servo;

//----------Config-----------
#define MyApiKey "5c750819-2e7b-420e-a2f0-8e06591ff4cb"         // Your API key
#define MySSID "Carter WiFi"                  // Your WIFI name
#define MyWifiPassword "tangomango"         // Your WIFI password
#define powerBtn "60749b86c26766757ec78950"         // Power Button ID
#define aromaBtn "60762345c26766757ec7c73d"         // Aroma Button ID
#define sugarBtn "607af256c26766757ec8b8d2"         // AddSugar Button ID
#define TIME_HEADER "T"                             // Header tag for serial time sync message
#define TIME_REQUEST 7                              // Ascii character requests a time sync message


int Aroma = 2;
int Power = 0;
int Sugar = 0;
const byte servoPin = 0;

int s=0;    //here you set the secs
int m=0;   //here you set the mins
int h=12;   //here you set the hour

//-------------------------
#define HEARTBEAT_INTERVAL 300000 // 5 Minutes 

uint64_t heartbeatTimestamp = 0;
bool isConnected = false;

void setPowerStateOnServer(String deviceId, String value);
//-------------------------------------------------------------------------------------------------
void turnOn(String deviceId) {  
  if (deviceId == aromaBtn) // Aroma Button ID
  {  
    Serial.print("Pouring some coffee for you");
//  lcd.print("Pouring some coffee for you...");
    Serial.println(deviceId);
    digitalWrite(Aroma, LOW);
    delay(500);
    digitalWrite(Aroma, HIGH);
    setPowerStateOnServer(aromaBtn, "OFF"); //Aroma Button ID
  }    
   if (deviceId == powerBtn) // Power Button ID
  {  
    Serial.print("Power button is clicked");
//  lcd.print("Pouring some coffee for you...");
    Serial.println(deviceId);
    digitalWrite(Power, LOW);
    delay(500);
    digitalWrite(Power, HIGH);
    setPowerStateOnServer(powerBtn, "OFF");  // Power Button ID
  }
  if (deviceId == sugarBtn) // Power Button ID
  {  
    Serial.print("Sugar is being added");
//  lcd.print("Adding sugar");
    Serial.println(deviceId);
    servo.write(0);
    delay(175);
    servo.write(90);
    setPowerStateOnServer(sugarBtn, "OFF");  // Sugar Button ID
  }
}

void turnOff(String deviceId) {
   if (deviceId == aromaBtn) // Aroma Button ID
   {  
    Serial.print("Pouring some coffee for you");
//  lcd.print("Pouring some coffee for you...");
    Serial.println(deviceId);
    digitalWrite(Aroma, LOW);
    delay(500);
    digitalWrite(Aroma, HIGH);
    setPowerStateOnServer(aromaBtn, "OFF"); // Aroma Button ID
   }  
   if (deviceId == powerBtn) // Power Button ID
   {  
    Serial.print("Power button is clicked");
//  lcd.print("Pouring some coffee for you...");
    Serial.println(deviceId);
    digitalWrite(Power, LOW);
    delay(500);
    digitalWrite(Power, HIGH);
    setPowerStateOnServer(powerBtn, "OFF"); // Power Button ID
   } 
   if (deviceId == sugarBtn) // Power Button ID
  {  
    Serial.print("Sugar stopped");
//  lcd.print("Sugar Stopped");
    Serial.println(deviceId);
    servo.write(180);
    delay(175);
    servo.write(90);
    setPowerStateOnServer(sugarBtn, "OFF");  // Sugar Button ID
  }
}
//------------------------------------------------------------------------------------------------
// Messages consist of the letter T followed by ten digit time (as seconds since Jan 1 1970)
// you can send the text on the next line using Serial Monitor to set the clock to time of upload 

void selfCleaningClock() {
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year());
  Serial.println();
  }

void printDigits(int digits) {
  Serial.print(":");
  if (digits < 10)
  Serial.print('0');
  Serial.print(digits);
}

void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1619037407; //Current unix epoch time - 21st April 9:37pm 2021
  
  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
       setTime(pctime); // Sync Arduino clock to the time received on the serial port
     }
  }
}

time_t requestSync()
{
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}
/*void selfCleanCycle(){
 if () {
    
    }
  }
*/
//------------------------------------------------------------------------------------------------
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      isConnected = false;    
      Serial.printf("[WSc] Webservice disconnected from sinric.com!\n");
  //  lcd.print("Webservice disconnected from sinric.com");
      break;
    case WStype_CONNECTED: {
      isConnected = true;
      Serial.printf("[WSc] Service connected to sinric.com at url: %s\n", payload);
      Serial.printf("Waiting for commands from sinric.com ...\n"); 
  //  lcd.print("Service connected to sinric.com");       
      }
      break;
    case WStype_TEXT: {
        Serial.printf("[WSc] get text: %s\n", payload);
          
#if ARDUINOJSON_VERSION_MAJOR == 5
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject((char*)payload);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
        DynamicJsonDocument json(1024);
        deserializeJson(json, (char*) payload);      
#endif        
        String deviceId = json ["deviceId"];     
        String action = json ["action"];
        
        if(action == "setPowerState") { // Switch or Light
            String value = json ["value"];
            if(value == "ON") {
                turnOn(deviceId);
            } else {
                turnOff(deviceId);
            }
        }
        else if (action == "test") {
            Serial.println("[WSc] received test command from sinric.com");
        //  lcd.print("Test");       

        }
      }
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      break;
  }
}

void setup() {
//lcd.begin(16, 2);
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT); // LED indicator - if we need to timesync
  setSyncProvider (requestSync); // set fucntion to call when sync is needed
  Serial.println ("Waiting for sync.");
  
  pinMode(Aroma, OUTPUT);
  pinMode(Power, OUTPUT);
  pinMode(Sugar, OUTPUT);
  servo.attach(servoPin);
  digitalWrite(Aroma, HIGH); // immediately setting them high to avoid miss-press
  digitalWrite(Power, HIGH); // immediately setting them high to avoid miss-press
  Serial.println("Barista.io is ready");

  WiFiMulti.addAP(MySSID, MyWifiPassword);
  Serial.println();
  Serial.print("Connecting to Wifi: ");
  Serial.println(MySSID);  

  // Waiting for Wifi connect
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if(WiFiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi connected. ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  // server address, port and URL
  webSocket.begin("iot.sinric.com", 80, "/");

  // event handler
  webSocket.onEvent(webSocketEvent);
  webSocket.setAuthorization("apikey", MyApiKey);
  
  // try again every 5000ms if connection has failed
  webSocket.setReconnectInterval(5000);   // If you see 'class WebSocketsClient' has no member named 'setReconnectInterval' error update arduinoWebSockets
}

void loop() {
//lcd.setCursor(0, 1);
  webSocket.loop();
  if(isConnected) {
      uint64_t now = millis();
      
      // Send heartbeat in order to avoid disconnections during ISP resetting IPs over night. Thanks @MacSass
      if((now - heartbeatTimestamp) > HEARTBEAT_INTERVAL) {
          heartbeatTimestamp = now;
          webSocket.sendTXT("H");          
      }
  }
  if (Serial.available()){
    processSyncMessage();
    }  
  if (timeStatus() != timeNotSet) {
    selfCleaningClock();
    } 
  if (timeStatus() == timeSet) {
    digitalWrite(LED_BUILTIN, HIGH); // LED on if synced properly
    }
  else {
    digitalWrite(LED_BUILTIN, LOW); //LED off if sync is needed
    }
  delay(1000);
}
 
void setPowerStateOnServer(String deviceId, String value) {
#if ARDUINOJSON_VERSION_MAJOR == 5
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
  DynamicJsonDocument root(1024);
#endif        

  root["deviceId"] = deviceId;
  root["action"] = "setPowerState";
  root["value"] = value;
  StreamString databuf;
#if ARDUINOJSON_VERSION_MAJOR == 5
  root.printTo(databuf);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6        
  serializeJson(root, databuf);
#endif  
  webSocket.sendTXT(databuf);
}

 

