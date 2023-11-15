/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-websocket-server-arduino/
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

// Import required libraries
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "index_html.h"

#define __DEV_LOG
#define __DEV_TEST
#define WIFI_SCAN_LIST 1

// network credentials
const char* ap_esp_ssid     = "ESP32-Access-Point";
const char* ap_esp_password = "123456789";
bool is_scan = false;

bool ledState = 0;
const int ledPin = 2;

DynamicJsonDocument buff(1024);

String ap_ssid;
String ap_pass;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void notifyClients() {
  ws.textAll(String(ledState));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle") == 0) 
    {
      ledState = !ledState;
      
      notifyClients();
    };
    
    if (strcmp((char*)data, "scan") == 0)
    {
      is_scan = true;
    };
    
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if (ledState){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
  return String();
}

void esp_scan_wifi()
{
  if (is_scan)
  {
    Serial.println("Scanning…");
    int n = WiFi.scanNetworks();
    Serial.println("scan done");

    if (n == 0) {
        Serial.println("no networks found");
        buff["type"] = WIFI_SCAN_LIST;
        buff["number"] = 0;

    } else {
      Serial.print(n);
      Serial.println(" networks found");

      buff["type"] = WIFI_SCAN_LIST;
      buff["number"] = n;

      for (int i = 0; i < n; ++i) {
#ifdef __DEV_LOG
        // Print SSID and RSSI for each network found
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(")");
        Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
#endif /*__DEV_LOG*/

        buff["ssid"][i] = WiFi.SSID(i);
        buff["rssi"][i] = WiFi.RSSI(i);
      }
    }
    Serial.println("");

    String msg;
    serializeJsonPretty(buff, msg);
    ws.textAll(String(msg));
    // Serial.println(msg);

    is_scan = false;
    // serializeJsonPretty(buff, Serial);

  }
  
}

#ifdef __DEV_TEST
bool SPIFFS_test(const String& path)
{
  File file = SPIFFS.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return false;
  }
  
  Serial.println("File Content:");
  while(file.available()){
    Serial.write(file.read());
  }

  file.close();
  Serial.println("[End of file]");

  return true;
}
#endif /*__DEV_TEST*/

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
#ifdef __DEV_LOG
    Serial.println("An Error has occurred while mounting SPIFFS");
#endif /*__DEV_LOG*/
    return;
  }

#ifdef __DEV_TEST
  SPIFFS_test("/text.txt");
#endif /*__DEV_TEST*/

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
#ifdef __DEV_LOG
  Serial.println("Setting AP (Access Point)…");
#endif /*__DEV_LOG*/

  WiFi.softAP(ap_esp_ssid, ap_esp_password);

#ifdef __DEV_LOG
  Serial.print("SSID AP: ");
  Serial.println(ap_esp_ssid);
  Serial.print("Password AP: ");
  Serial.println(ap_esp_password);
#endif /*__DEV_LOG*/

  IPAddress IP = WiFi.softAPIP();

#ifdef __DEV_LOG
  Serial.print("AP IP address: ");
  Serial.println(IP);
#endif /*__DEV_LOG*/

  initWebSocket();

  // Route for root / web page
  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/html", index_html, processor);
  // });

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Route to load script.js file
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/script.js", "text/js");
  });

  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost()){
        // HTTP POST ssid value
        if (p->name() == "ssid") {
          ap_ssid = p->value().c_str();
          Serial.print("SSID set to: ");
          Serial.println(ap_ssid);
          // Write file to save value
          // writeFile(SPIFFS, ssidPath, ssid.c_str());
        }
        // HTTP POST pass value
        if (p->name() == "pass") {
          ap_pass = p->value().c_str();
          Serial.print("Password set to: ");
          Serial.println(ap_pass);
          // Write file to save value
          // writeFile(SPIFFS, passPath, pass.c_str());
        }
      }
    }
  });

#ifdef __DEV_TEST
  server.on("/hello", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Hello World");
  });
#endif /*__DEV_TEST*/

  // Start server
  server.begin();
}



void loop() {
  ws.cleanupClients();
  digitalWrite(ledPin, ledState);
  esp_scan_wifi();
}
