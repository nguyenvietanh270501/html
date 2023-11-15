#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "StringArray.h"
#include <ArduinoJson.h>
#include <EEPROM.h>

#define EEPROM_SIZE 64
// Replace with your network credentials
const char* ssid = "hyperlogy";
const char* password = "123456789";
bool is_scan = false;
int eepromOffset = 0;

unsigned long start_time= millis();

DynamicJsonDocument buff(1024);

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void connectWiFi(String ssid_ap, String pw_ap);
void connectWiFi_eeprom();


struct WifiNetwork {
    String ssid;
    int rssi;
    int encryptionType;
};
int compareNetworks(const void* a, const void* b) {
    return ((WifiNetwork*)b)->rssi - ((WifiNetwork*)a)->rssi;
}
void sortNetworksByRSSI(WifiNetwork* networks, int numNetworks) {
    qsort(networks, numNetworks, sizeof(WifiNetwork), compareNetworks);
}
void scanAndSortWifiNetworks() {
    if (is_scan == true)
    {
        Serial.println("scan start");

    // WiFi.scanNetworks will return the number of networks found
    int numNetworks = WiFi.scanNetworks();
    Serial.println("scan done");

    if (numNetworks == 0) {
        Serial.println("No networks found");
    }
    else {
        WifiNetwork* networks = new WifiNetwork[numNetworks];

        for (int i = 0; i < numNetworks; ++i) {
            networks[i].ssid = WiFi.SSID(i);
            networks[i].rssi = WiFi.RSSI(i);
            networks[i].encryptionType = WiFi.encryptionType(i);
        }

        // Sắp xếp mạng Wi-Fi theo RSSI (yếu đến mạnh)
        sortNetworksByRSSI(networks, numNetworks);

        Serial.println("Networks found (sorted by RSSI strength):");
        for (int i = 0; i < numNetworks; ++i) {
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(networks[i].ssid);
            Serial.print(" (");
            Serial.print(networks[i].rssi);
            Serial.print(")");
            Serial.println((networks[i].encryptionType == WIFI_AUTH_OPEN) ? " " : "*");
            delay(10);
            }
            for (int i = 0; i < numNetworks; ++i) {
                buff["ssid"][i] = networks[i].ssid;
            }

         String msg;
        serializeJsonPretty(buff, msg);
        ws.textAll(msg);
        is_scan =false;   
    }
    }
    
}

int writeWiFiToEEPROM(int addrOffset, const String &strToWrite)
{
  EEPROM.begin(EEPROM_SIZE);
  byte len = strToWrite.length();
  EEPROM.write(addrOffset, len);
  for (int i = 0; i < len; i++)
  {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
  }
  EEPROM.commit();
  return addrOffset + 1 + len;
}

int readWiFiFromEEPROM(int addrOffset, String *strToRead)
{
  EEPROM.begin(EEPROM_SIZE);
  int newStrLen = EEPROM.read(addrOffset);
  char data[newStrLen + 1];
  for (int i = 0; i < newStrLen; i++)
  {
    data[i] = EEPROM.read(addrOffset + 1 + i);
  }
  data[newStrLen] = '\0';
  *strToRead = String(data);
  return addrOffset + 1 + newStrLen;
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>CONNECT</title>
    <style>
        body {
            background-color: burlywood;
        }

        body {
            text-align: center;
            font-family: Arial,sans-serif;
        }

        h1 {
            margin-top: 40px;
        }

        .container {
            display: flex;
            flex-direction: column;
            align-items: center;
        }
        table-wifi {
            width: 30%;
            border-collapse: collapse;
            margin-top: 10px;
            border-color: transparent;
        }
        #table-wifi tr:nth-child(even){background-color: #f2f2f2;}

        #table-wifi tr:hover {background-color: #ddd;}

        small-table{
          width: 50%;
          border-collapse: collapse;
          margin-top: 5px;
          border-color: transparent;

        }

        td {
            border: 1px solid #ccc;
            padding: 5px;
            color: black;
        }

        .button {
            margin-top: 1px;
        }
    </style>
</head>
<body>
  <h1>-------------------- </h1>
  <div class="container">
      <table class="table-wifi">
          <tr>
              <td><span id="wifi1">%WiFi1%</span></td>
              <td><button class="button" onclick="fillUsername('button1')">Connect</button></td>
          </tr>
          <tr>
              <td><span id="wifi2">%WiFi2%</span></td>
              <td><button class="button" onclick="fillUsername('button2')">Connect</button></td>
          </tr>
          <tr>
              <td><span id="wifi3">%WiFi3%</span></td>
              <td><button class="button" onclick="fillUsername('button3')">Connect</button></td>
          </tr>
          <tr>
            <td><span id="wifi4">%WiFi4%</span></td>
            <td><button class="button" onclick="fillUsername('button4')">Connect</button></td>
        </tr>
        <tr>
            <td><span id="wifi5">%WiFi5%</span></td>
            <td><button class="button" onclick="fillUsername('button5')">Connect</button></td>
        </tr>
    </table>
</div>
<form action="">
    User name:<br>
    <input type="text" name="userid" id="username">
    <br>
    User password:<br>
    <input type="password" name="psw" id="password">
</form>
<div class="container">
  <table class="small-table">
    <tr>
      <td><button class="button" onclick="scan()">Scan</button></td>
      <td><button class="button" onclick="connect()">Save</button></td>
    </tr>
  </table>
</div>
<script>
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
      console.log('Trying to open a WebSocket connection...');
      websocket = new WebSocket(gateway);
      websocket.onopen = onOpen;
      websocket.onclose = onClose;
      websocket.onmessage = onMessage;
  }
  function onOpen(event) {
      console.log('Connection opened');
  }
  function onClose(event) {
            console.log('Connection closed');
            setTimeout(initWebSocket, 2000);
        }
        function fillUsername(name) {
            if (name == "button1") {
                document.getElementById("username").value = document.getElementById("wifi1").textContent;
            } else if (name == "button2") {
                document.getElementById("username").value = document.getElementById("wifi2").textContent;
            } else if (name == "button3") {
                document.getElementById("username").value = document.getElementById("wifi3").textContent;
            } else if (name == "button4") {
                document.getElementById("username").value = document.getElementById("wifi4").textContent;
            } else if (name == "button5") {
                document.getElementById("username").value = document.getElementById("wifi5").textContent;
            }
        }
        function onMessage(event) {
            const receivedData = JSON.parse(event.data);
            if (receivedData.ssid) {
                const ssidArray = receivedData.ssid;
                document.getElementById('wifi1').innerHTML = ssidArray[0];
                document.getElementById('wifi2').innerHTML = ssidArray[1];
                document.getElementById('wifi3').innerHTML = ssidArray[2];
                document.getElementById('wifi4').innerHTML = ssidArray[3];
                document.getElementById('wifi5').innerHTML = ssidArray[4];
            }
        }
        function onLoad(event) {
            initWebSocket();
            initButton();
        }
        function initButton() {
            document.getElementById('saveButton').addEventListener('click', savePass);
        }
        function scan() {
            websocket.send('scan');
        }
        function login(sta_ssid, sta_pass) {
            const wifiCredentials = { ssid: sta_ssid, password: sta_pass };
            const jsonCredentials = JSON.stringify(wifiCredentials);
            websocket.send(jsonCredentials);
        }
        function connect() {
            const sta_ssid = document.getElementById("username").value;
            let sta_pass = document.getElementById("password").value;
            login(sta_ssid, sta_pass);
        }
    </script>
</body>
</html>
)rawliteral";

/*void notifyClients(String) {
  ws.textAll(String);
}*/

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "scan") == 0) {
        is_scan = true;
      
     // notifyClients();
    }
    else{
        DynamicJsonDocument jsonDocument(1024);
        deserializeJson(jsonDocument, (char *)data);
        if (jsonDocument.containsKey("ssid") && jsonDocument.containsKey("password")) {
      const char *ssid = jsonDocument["ssid"];
      const char *password = jsonDocument["password"];

      // Now you have ssid and password, you can use them in your application
      Serial.print("Received SSID: ");
      Serial.println(ssid);
      Serial.print("Received Password: ");
      Serial.println(password);
      connectWiFi(ssid,password);
      connectWiFi_eeprom();
    } else {
      Serial.println("Invalid or missing ssid/password in the received JSON data.");
    }

    }
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

String processor(const String& var)
{
  Serial.println(var);
  if(var == "WiFi1"){

    return "";
  }
  else if (var == "WiFi2"){
    return "";
  }
  else if (var == "WiFi3"){
    return "";
  }
  else if (var == "WiFi4"){
    return "";
  }
  else if (var == "WiFi5"){
    return "";
  }
}

void connectWiFi(String ssid_ap, String pw_ap){
    start_time=millis();
    Serial.println("Connecting to WiFi...");
    Serial.println(ssid_ap);
     WiFi.begin(ssid_ap,pw_ap );

  while (WiFi.status() != WL_CONNECTED && millis() - start_time < 60000) {
    delay(1000);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
  int str1AddrOffset = writeWiFiToEEPROM(eepromOffset, ssid_ap);
  writeWiFiToEEPROM(str1AddrOffset, pw_ap);
  } else {
    Serial.println("\nFailed to connect within 1 minute. Stopping connection attempts.");
    WiFi.disconnect(true);
  }

}

void connectWiFi_eeprom(){
  String ssid_eeprom;
  String pw_eeprom;
  int newStr1AddrOffset = readWiFiFromEEPROM(eepromOffset, &ssid_eeprom);
  int newStr2AddrOffset = readWiFiFromEEPROM(newStr1AddrOffset, &pw_eeprom);
  Serial.println(ssid_eeprom);
  Serial.println(pw_eeprom);
  connectWiFi(ssid_eeprom,pw_eeprom);
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  connectWiFi_eeprom();

  WiFi.softAP(ssid,password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Start server
  server.begin();
}

void loop() {
  ws.cleanupClients();
  scanAndSortWifiNetworks();
}