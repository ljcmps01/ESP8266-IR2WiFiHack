/*Conexion:
 * Data=(D2)GPIO4
 * 
 */
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

const char* ssid = "WiFi ID";
const char* password = "Wifi password";
const char* mqtt_server = "broker.hivemq.com";
const char* MQTT_topic = "your/topic";        //eg. "home/bedroom/lights"
const char* mqtt_user = "blank if none";
const char* mqtt_pass = "blank if none";

WiFiClient espClient;
PubSubClient client(espClient);


const int nCodes=16;
const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

String palabras[nCodes]={"on","off","-","+","white","red","green","blue","violet","pink","flash","smooth","fade","yellow","light blue","lime"};
              //ON            Off           -           +             Blanco      Rojo        Verde         Azul        Violeta     Rosa          Flash       Smooth        Fade
int codes[nCodes]={
  0x00F7C03FUL,
  0x00F740BFUL,
  0x00F7807FUL,
  0x00F700FFUL,
  0x00F7E01FUL,
  0x00F720DFUL,
  0x00F7A05FUL,
  0x00F7609FUL,
  0x00F7708FUL,
  0x00F76897UL,
  0x00F7D02FUL,
  0x00F7E817UL,
  0x00F7C837UL,
  0x00F728D7UL,
  0x00F78877UL,
  0x00F7A857UL
  };

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

void setup() {
  //initialize the IR library
  irsend.begin();
#if ESP8266
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
#else  // ESP8266
  Serial.begin(115200, SERIAL_8N1);
#endif  // ESP8266  

  //connects to WiFi
  setup_wifi();

  //Establish connection with the MQTT broker and set the callback function
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial.println("MQTT ready");

  //Start the OTA setup
  if (!MDNS.begin("NodeMCU-LEDs")) {  //Change the name to whatever you want
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

  //Once everything is set up, we print the IP adress and make the lights blink quickly once
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  irsend.sendNEC(codes[0]);
  delay(100);
  irsend.sendNEC(codes[4]);
  delay(100);
  irsend.sendNEC(codes[1]);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),mqtt_user,mqtt_pass)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("", "hello world");
      // ... and resubscribe
      client.subscribe(MQTT_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  ArduinoOTA.handle();
}

void callback(char* topic, byte* payload, unsigned int length) {
  //Saves the payload into a String
  byte *m=NULL;
  m=payload;
  String receivedStr;
  for(int i=0;i<length;i++){
    receivedStr.concat(String (char(*(m+i))));
  }
  //Transforms everything to lower case to avoid spelling errors
  receivedStr.toLowerCase();

  //Compare the string received to the keywords to trigger the code
  for(int i=0;i<nCodes;i++){
    //If the words matches it sends the code on the same ubication
    if (receivedStr==palabras[i]){
      irsend.sendNEC(codes[i]);
      Serial.print("signal received: "+receivedStr);

      //If the code corresponds to the brigthness it sends it a bunch of times
      //So we don't have to send the same signal too many times
      if(i==2||i==3){
        for(int j=0;j<25;j++){
          irsend.sendNEC(codes[i]);
        }
      }
    }
  }
}
