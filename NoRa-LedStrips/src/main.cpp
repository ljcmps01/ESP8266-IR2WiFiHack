/*Google Home sends a JSON string throught MQTT with the following format:
 * {
 *  "online":boolean,     --Checks if the device is connected (useless for our code
 *  "on":boolean,         --The state the device should be (True if it's on, false if it's off)
 *  "brightness":int,     --A percentage of how bright the lights should be, from 0 (min) to 100(max)
 *  "color":{             --A JSON object that contains the information of how the color should be following these parameters
 *    "spectrumHsv"{
 *      "hue":int,          --Represents a circle of colors (in degrees) from 0 to 359
 *      "saturation":float, --Dunno, not used
 *      "value":1           --Dunno, always = 1
 *    }
 * }
 */
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <EEPROM.h>

// Update these with values suitable for your network.

const char* ssid = "Campos";
const char* password = "perico15";
const char* mqtt_server = "raspberrypi.local";

const int nCodes=16;    //Number of codes extracted from the IR controller
const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use (must be PWM). Recommended: 4 (D2).
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

//These pair of arrays contains the trigger keywords and their respective code for said function
String palabras[nCodes]={"on","off","-","+","white","red","green","blue","violet","pink","flash","smooth","fade","yellow","light blue","lime"};
              //ON            Off           -           +             Blanco      Rojo        Verde         Azul        Violeta     Rosa          Flash       Smooth        Fade
int codes[nCodes]={
  0x00F7C03FUL, //ON 0
  0x00F740BFUL, //Off 1
  0x00F7807FUL, //- 2
  0x00F700FFUL, //+ 3
  0x00F7E01FUL, //White 4
  0x00F720DFUL, //Red 5
  0x00F7A05FUL, //Blue 6
  0x00F7609FUL, //Violet 7
  0x00F7708FUL, //Pink 8
  0x00F76897UL, //Flash 9
  0x00F7D02FUL, //Smooth 10
  0x00F7E817UL, //Fade 11
  0x00F7C837UL, //Yellow 12
  0x00F728D7UL, //Light Blue 13 
  0x00F78877UL, //Lime 14
  0x00F7A857UL  //
  };


WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

//Evaluation of the incoming data and comparation with its previous state and values
bool oldstate=0;
int  oldbrightness=0;
int oldcolor=0;
void evaluateColor();
void evaluateInstruction(bool state, int brightness, int color){
  if (oldstate!=state){
    Serial.println("State change received");
    state?irsend.sendNEC(codes[0]):irsend.sendNEC(codes[1]);
    oldstate=state;
    Serial.println("New State: "+ String(state));
  }

  if (oldbrightness!=brightness)
  {
    Serial.println("Brightness change received");
    if (oldbrightness>brightness)
    { 
      Serial.print("Dimming the lights to: ");
      Serial.println(brightness);
      for(oldbrightness;oldbrightness!=brightness;oldbrightness--)
      {
        Serial.print(". -- ");
        Serial.println(oldbrightness);
        irsend.sendNEC(codes[2]);
      }
    }
    else
    {
      Serial.print("Brightening the lights to: ");
      Serial.println(brightness);
      for(oldbrightness;oldbrightness!=brightness;oldbrightness++)
      {
        Serial.print(". -- ");
        Serial.println(oldbrightness);
        irsend.sendNEC(codes[3]);
      }
    }
    EEPROM.write(0,brightness);
    EEPROM.commit();
    Serial.print("New brightness");
    Serial.println(brightness);
  }
  
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  StaticJsonDocument<256> doc;  
  
  Serial.print("payload: ");
  for(int i =0; i<length; i++){
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
  deserializeJson(doc, payload, length);

  Serial.print("status: ");
  bool On=doc["on"];
  Serial.println(On);
  
  Serial.print("Brillo: ");
  int brightness=doc["brightness"];
  Serial.println(brightness);
  
  Serial.print("color: ");
  float color=doc["color"]["spectrumHsv"]["hue"];
  Serial.println(color);
  
  Serial.print("saturation: ");
  float saturation=doc["color"]["spectrumHsv"]["saturation"];
  Serial.println(saturation);
  
  Serial.print("ilumination: ");
  float ilumination=doc["color"]["spectrumHsv"]["value"];
  Serial.println(ilumination);

  Serial.print("producto: ");
  int producto=color*saturation*ilumination;
  Serial.println(producto);

  evaluateInstruction(On,brightness,producto);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("", "hello world");
      // ... and resubscribe
      client.subscribe("/home/bedroom/ligths/ledstrips");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(4);
  oldbrightness=EEPROM.read(0);
 
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  client.setBufferSize(512); 
  if (!MDNS.begin("NodeMCU01")) {             // Start the mDNS responder for esp8266.local
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //Once everything of the network is setup we send a few commands to make the lights blink
  //And let us know that everything fine
  irsend.sendNEC(codes[0]); //Turn on the leds
  delay(100);
  irsend.sendNEC(codes[4]); //Make them white
  delay(100);
  irsend.sendNEC(codes[1]); //Turn off the leds

  Serial.print("Current brightness: ");
  Serial.println(oldbrightness);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  ArduinoOTA.handle();
}
