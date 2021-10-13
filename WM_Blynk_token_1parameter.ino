/*************************************************************************
 * Title: WiFiManager and Blynk Token Input 
 * File: WM_Blynk_token_1parameter.ino
 * Author: Marcelo Oggero
 * Date: 10/10/2021
 *
 * This program controls a ESP8266 Mini board with a power output relay (220VAC 50A). The module is controlled from the
 * internet via the Blynk cloud app that can read a momentary pushbutton input to change relay status. Alco can receive status change throught Blynk app 
 * 
 * Notes:
 *  (1) Requires the following arduino libraries:
 *      ESP8266 2.7.4
 *      Blynk 1.0.1
 *      WiFiManager 2.0.4 Beta 
 *      ArduinoJson 5.13.5
 *  (2) Compiled with arduino ide 1.8.12
 *  (3) Uses three Blynk app widgets:
 *       V0: button configured as a switch.
 *       V1: led.
 *       V2: led.
 *************************************************************************
 * Change Log:
 *   12/25/2016: Initial release. JME
 *   12/31/2016: Added input pin status. JME
 *   01/15/2017: Added volatile. JME
 *************************************************************************/
#include <FS.h> //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          // https://github.com/bblanchon/ArduinoJson
 
// Esp8266 pins.
#define ESP8266_GPIO2    2 // Blue LED.
#define ESP8266_GPIO4    4 // Relay control.
#define ESP8266_GPIO0    0 // input.
#define LED_PIN          ESP8266_GPIO2

    
// Flag for sync on re-connection.
bool isFirstConnect = true; 
volatile int relayState = LOW;    // Blynk app pushbutton status.
volatile int inputState = LOW;    // Input pin state.

/////////////////////////////////////////////////////////////////////////////////////////PARA WIFIMANAGER
// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String outputState = "off";

// Assign output variables to GPIO pins

char blynk_token[34] = "YOUR_BLYNK_TOKEN";

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

/////////////////////////////////////////////////////////////////////////////////////////FIN PARA WIFIMANAGER








//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////                                                      SETUP
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void setup() {
  pinMode( ESP8266_GPIO4, OUTPUT );       // Relay control pin.
  pinMode( ESP8266_GPIO0, INPUT_PULLUP ); // Input pin.
  pinMode( LED_PIN, OUTPUT );             // ESP8266 module blue LED.
  digitalWrite( LED_PIN, LOW );           // Turn on LED.
  digitalWrite( LED_PIN, HIGH );          // Turn off LED.


/////////////////////////////////////////////////////////////////////////////////////////PARA WIFIMANAGER
Serial.begin(115200);
  
  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          strcpy(blynk_token, json["blynk_token"]);
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
  
  
  WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 35);

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  
  // set custom ip for portal
  //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  //add all your parameters here
  wifiManager.addParameter(&custom_blynk_token);
  
  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("AutoConnectAP");
  // or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();
  
  // if you get here you have connected to the WiFi
  Serial.println("Connected.");
   
  strcpy(blynk_token, custom_blynk_token.getValue());
  
  Serial.println("The values in the file are: ");
  Serial.println("\tblynk_token : " + String(blynk_token));


  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    
    json["blynk_token"] = blynk_token;
    
    
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  // Initialize the output variables as outputs
  pinMode(4, OUTPUT);
  // Set outputs to LOW
  digitalWrite(4, LOW);;
  
  server.begin();
/////////////////////////////////////////////////////////////////////////////////////////FIN PARA WIFIMANAGER


  Blynk.begin( blynk_token, WiFi.SSID().c_str(), WiFi.psk().c_str() );    // Initiate Blynk conection.

  
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                    FIN SETUP
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 
// This function runs every time Blynk connection is established.
BLYNK_CONNECTED() {
  if ( isFirstConnect ) {
    Blynk.syncAll();
    isFirstConnect = false;
  }
}
 
// Sync input LED.
BLYNK_READ( V2 ) {
  CheckInput();
}
 
// Blynk app relay command.
BLYNK_WRITE( V0 ) {
  if ( param.asInt() != relayState ) {
    relayState = !relayState;                  // Toggle state.
    digitalWrite( ESP8266_GPIO4, relayState ); // Relay control pin.
    Blynk.virtualWrite( V1, relayState*255 );  // Set Blynk app LED.
  }
}
 
// Debounce input pin.
int DebouncePin( void ) {
  // Read input pin.
  if ( digitalRead( ESP8266_GPIO0 ) == HIGH ) {
    // Debounce input.
    delay( 25 );
    if ( digitalRead( ESP8266_GPIO0 ) == HIGH )
      return HIGH;
  }
  return LOW;
}
 
// Set LED based upon state of input pin.
void CheckInput( void ) {
  if ( DebouncePin() != inputState ) {
    Blynk.virtualWrite( V2, inputState*255 );
    inputState = !inputState;
  }
}
 
// Main program loop.
void loop() {
  //Serial.println("SSID mioooooo"+ WiFi.SSID());
  //Serial.println("psk mioooooo"+ WiFi.psk());
  Blynk.run();
  CheckInput();
  //yield(); //Updated: 3/8/2017
}
