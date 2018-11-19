/*
  MIT License.
  Install MQTT lib to ESP8266.
  Tested on NodeMCU 1.0 (ESP12E)
*/

#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#define AIO_SERVER      "mqtt-server.org"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "mqtt-username"
#define AIO_KEY         "mqtt-password"

const char* ssid     = "SSID";
const char* password = "WiFi-password";

const byte s8_co2[8] = {0xfe, 0x04, 0x00, 0x03, 0x00, 0x01, 0xd5, 0xc5};
const byte s8_fwver[8] = {0xfe, 0x04, 0x00, 0x1c, 0x00, 0x01, 0xe4, 0x03};
const byte s8_id_hi[8] = {0xfe, 0x04, 0x00, 0x1d, 0x00, 0x01, 0xb5, 0xc3};
const byte s8_id_lo[8] = {0xfe, 0x04, 0x00, 0x1e, 0x00, 0x01, 0x45, 0xc3};
SoftwareSerial swSer(13, 15, false, 256); // RX, TX

byte buf[10];
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish senseair = Adafruit_MQTT_Publish(&mqtt, "/feeds/co2");

void MQTT_connect();

void myread(int n) {
  int i;
  memset(buf, 0, sizeof(buf));
  for(i = 0; i < n; ) {
    if(swSer.available() > 0) {
      buf[i] = swSer.read();
      i++;
    }
    yield();
    delay(10);
  }
}

// Compute the MODBUS RTU CRC
uint16_t ModRTU_CRC(byte* buf, int len){
  uint16_t crc = 0xFFFF;
  
  for (int pos = 0; pos < len; pos++) {
    crc ^= (uint16_t)buf[pos];          // XOR byte into least sig. byte of crc
  
    for (int i = 8; i != 0; i--) {    // Loop over each bit
      if ((crc & 0x0001) != 0) {      // If the LSB is set
        crc >>= 1;                    // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                            // Else LSB is not set
        crc >>= 1;                    // Just shift right
    }
  }
  // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
  return crc;  
}

void setup() {
  Serial.begin(9600);
  swSer.begin(9600);
  delay(10);

  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Sensor ID: ");
  swSer.write(s8_id_hi, 8);
  myread(7);
  Serial.printf("%02x%02x", buf[3], buf[4]);
  swSer.write(s8_id_lo, 8);
  myread(7);
  Serial.printf("%02x%02x", buf[3], buf[4]);
  Serial.println("");

  swSer.write(s8_fwver, 8);
  myread(7);
  Serial.printf("Firmware: %d.%d", buf[3], buf[4]);
  Serial.println();
}

int value = 0;

void loop() {
  uint16_t co2;

  MQTT_connect();
  co2 = readco2();
  senseair.publish(co2);
  
  delay(10 * 1000L);
}

uint16_t readco2() {
  uint16_t crc, got, co2;
  
  swSer.write(s8_co2, 8);
  myread(7);
  co2 = (uint16_t)buf[3] * 256 + (uint16_t)buf[4];
  crc = ModRTU_CRC(buf, 5);
  got = (uint16_t)buf[5] + (uint16_t)buf[6] * 256;
  if(crc != got) {
    Serial.print("Invalid checksum.  Expect: ");
    Serial.print(crc, HEX);
    Serial.print("  Got: ");
    Serial.println(got, HEX);
  } else {
    Serial.print("CO2: ");
    Serial.println(co2);
    // Serial.printf("%02x %02x %02x %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);
  }
  return co2;
}

void MQTT_connect() {
  int8_t ret;

  if(mqtt.connected())  return;

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while((ret = mqtt.connect()) != 0) {
   Serial.println(mqtt.connectErrorString(ret));
   Serial.println("Retrying MQTT connection in 5 seconds...");
   mqtt.disconnect();
   delay(5000);  // wait 5 seconds
   retries--;
   if(retries == 0) {
    Serial.println("Cannot connect to MQTT.  System hangs.");
     while(1);
   }
  }
  Serial.println("MQTT Connected!");
}
