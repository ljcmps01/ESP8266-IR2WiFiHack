/*Conexion:
 * Data=(D2)GPIO4
 * 
 */

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ESPHelper.h>

const int nCodes=16;
volatile bool dato=false;

netInfo homeNet1 = {  .mqttHost = "io.adafruit.com",     //can be blank if not using MQTT
          .mqttUser = "ljcmps",   //can be blank
          .mqttPass = "ab2efbfbaaab49c18dfe51ce389d8e12",   //can be blank
          .mqttPort = 1883,         //default port for MQTT is 1883 - only chance if needed.
          .ssid = "Campos", 
          .pass = "perico15"};

ESPHelper myESP(&homeNet1);

const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

String palabras[nCodes]={"on","off","-","+","white","red","green","blue","violet","pink","flash","smooth","fade","yellow","light blue","lime"};
              //ON            Off           -           +             Blanco      Rojo        Verde         Azul        Violeta     Rosa          Flash       Smooth        Fade
int codes[nCodes]={0x00F7C03FUL,0x00F740BFUL,0x00F7807FUL,0x00F700FFUL,0x00F7E01FUL,0x00F720DFUL,0x00F7A05FUL,0x00F7609FUL,0x00F7708FUL,0x00F76897UL,0x00F7D02FUL,0x00F7E817UL,0x00F7C837UL,0x00F728D7UL,0x00F78877UL,0x00F7A857UL};

void setup() {
  irsend.begin();
#if ESP8266
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
#else  // ESP8266
  Serial.begin(115200, SERIAL_8N1);
#endif  // ESP8266

  myESP.OTA_enable();
  myESP.OTA_setPassword("123");
  myESP.OTA_setHostname("ESP12F-LEDs");
  myESP.begin();
  
  myESP.addSubscription("ljcmps/feeds/room.leds");
  myESP.setMQTTCallback(callback);
  myESP.begin();
  Serial.println("MQTT ready");
  irsend.sendNEC(codes[0]);
  delay(100);
  irsend.sendNEC(codes[4]);
  delay(100);
  irsend.sendNEC(codes[1]);
}

void loop() {
  myESP.loop();
  yield();
}

void callback(char* topic, byte* payload, unsigned int length) {
  byte *m=NULL;
  m=payload;
  String receivedStr;
  for(int i=0;i<length;i++){
    receivedStr.concat(String (char(*(m+i))));
  }
  receivedStr.toLowerCase();
  for(int i=0;i<nCodes;i++){
    if (receivedStr==palabras[i]){
      irsend.sendNEC(codes[i]);
      String mensaje="Señal recibida de: ";
      mensaje.concat(receivedStr);
      Serial.print("Señal recibida: ");
      Serial.println(receivedStr);
      if(i==2||i==3){
        for(int j=0;j<25;j++){
          irsend.sendNEC(codes[i]);
        }
      }
    }
  }
}
