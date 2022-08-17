#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#include "time.h"

#ifndef STASSID
#define STASSID "nome"
#define STAPSK "senha"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;
int count = 0;
long int wait_time = 0;

double last_sync_lat = 0;
double last_sync_lon = 0;
double lat = 0;
double lon = 0;

#include <LoRa.h>
#define SS 15
#define RST 16
#define DIO0 4

String data = "";
String id = "00001";

void setup() {
  randomSeed(1);
  
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  
  while (!Serial);
  Serial.println("Device Activated");

  while (!internet_connection()){
    delay(3000);
    Serial.println("Reconectando...");
  }

  LoRa.setPins(SS, RST, DIO0);
  
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa Error");
    delay(100);
    while (1);
  }
  
  data.reserve(200);
  started();

  // LoRa Iniciado
  Serial.println("LoRa Started");
  digitalWrite(LED_BUILTIN, HIGH);

}
 
void loop() {
  WiFiClient client;
  
  long int t1 = millis();
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    while (LoRa.available()) {
      String received = LoRa.readString();
      String id_received = received.substring(0,5);
      if (id_received == id) break;
      blink();
      Serial.print("Message Received : ");
      Serial.println(received);
      processReceivedMsg(received);
      blink();
    }
  } else {
    delay(100);
  }
  long int t2 = millis();

  wait_time = wait_time + (t2 - t1);
  if (wait_time >= 60000) {
    String la = getLatitude();
    String lo = getLongitude();
    if (abs(CalcDistance(last_sync_lat, last_sync_lon, lat, lon)) > 500){
      last_sync_lat = lat;
      last_sync_lon = lon;
      if (WiFi.status() == WL_CONNECTED){
        Serial.println("Enviando Packet via Web");
        String json = createLocationPacketJson(id, getTimestamp(), la, lo);
        sendLocationPacket_server(json);
      } else {
        Serial.println("Enviando Packet via LoRa");
        sendLocationPacket(createLocationPacketUL(id, getTimestamp(), la, lo));
      }
    }
    wait_time = 0;
  }
}

void processReceivedMsg(String received) {
  WiFiClient client;
  
  String id = received.substring(0,5);
  String ts = received.substring(6,19);
  String la = received.substring(20,30);
  String lo = received.substring(31,41);
  
  if (WiFi.status() == WL_CONNECTED){
      Serial.println("Encaminhando msg via web...");
      Serial.println("id: " + id);
      Serial.println("ts: " + ts);
      Serial.println("la: " + la);
      Serial.println("lo: " + lo);
      sendLocationPacket_server(createLocationPacketJson(id, ts, la, lo));
   } else {
      Serial.println("Encaminhando msg via LoRa...");
      Serial.println("id: " + id);
      Serial.println("ts: " + ts);
      Serial.println("la: " + la);
      Serial.println("lo: " + lo);
      sendLocationPacket(createLocationPacketUL(id, ts, la, lo));
   }
}

void sendLocationPacket_server(String json){
  HTTPClient http;
  //WiFiClient client;
  WiFiClientSecure client;
  client.setInsecure(); //the magic line, use with caution
  client.connect("https://backend.thinger.io/", 80);

  Serial.println(json);
  http.begin(client, "https://backend.thinger.io/v3/users/{username}/devices/"+id+"/callback/data");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer xxx");
    
  int httpResponseCode = http.POST(json);
      
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
  }
  
  http.end();
}

void sendLocationPacket(String msg){
  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();
  Serial.println(msg);
}

String getLatitude() {
  double sum = double(random(1000) - 500) / 1000000.0;
  lat = lat + sum;
  return String(lat, 6);
}

String getLongitude() {
  double sum = double(random(1000) - 500) / 1000000.0;
  lon = lon + sum;
  return String(lon, 6);
}

String getTimestamp() {
  return String((int)time(NULL)) + "000";
}

String createLocationPacketUL(String id, String ts, String la, String lo){
  String msg = id + "|" + ts + "|" + la + "|" + lo + "\n";
  return msg;
}

String createLocationPacketJson(String id, String ts, String la, String lo){
  String json = "{\"id\":\""+ id +"\",\"ts\":\""+ ts +"\",\"lat\":\""+ la +"\",\"lon\":\""+ lo +"\"}";
  return json;
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    data += inChar;
    if (inChar == '\n') {
      Serial.print("Sending Message: ");
      Serial.println(data);
      LoRa.beginPacket();
      LoRa.print(data);
      LoRa.endPacket();
      data="";
      delay(2000);
    }
  }
}

void blink() {
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
}

void started() {
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);

    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);

    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
}

bool internet_connection(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    Serial.println("Connected to internet.");
    configTime(0, 3600, "pool.ntp.org");
    Serial.println("Sincronized timestamp.");
    return true;
  } else {
    Serial.println("Not connected to internet.");
    return false;
  }
}

double CalcDistance(double lat1, double lon1, double lat2, double lon2){
  double dlon = (lon2 - lon1) * 60 * 1852;
  double dlat = (lat2 - lat1) * 60 * 1852;
  double dist = sqrt( pow(dlon,2) + pow(dlat,2) );
  Serial.println(dist);
  return dist;
}
