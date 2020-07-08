# ESP8266-IR2WiFiHack
This repo coinstains all the data needed to connect your cheap IR LED strips to WiFi using an ESP8266 module and MQTT connection
## Work to be done
### NoRa-LedStrips
* Add/complete color evaluation function
## Working
### LEDStripsMQTT
The program on this file allows us to easily talk to the ESP trought MQTT to send codes to the receiver board
You can easily add semi-compatibility with Google Assistant/Home using Adafruit IO as the MQTT server and IFTTT to work as a bridge between Google and Adafruit
### NoRa-LedStrips
This one is similar to LEDStripsMQTT with the advantage that it can be configured to have full compatibility with Google Home using a local server running Node-Red with NoRa plugin and decoding the petitions of Google through a JSON File
Now it can receive the JSON file and read all the data on it, compare its current state and brightness and change the lights accordingly, however the colour is still in development as it is sent in a complex way, using the Spectrum HSV to define it
