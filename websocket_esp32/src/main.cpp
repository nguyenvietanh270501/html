#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "StringArray.h"

// Replace with your network credentials
const char* ssid = "hyperlogy";
const char* password = "123456789";
bool s = 1;

String wifi_name[5];

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

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
WifiNetwork* scanAndSortWifiNetworks() {
    Serial.println("scan start");

    // WiFi.scanNetworks will return the number of networks found
    int numNetworks = WiFi.scanNetworks();
    Serial.println("scan done");

    if (numNetworks == 0) {
        Serial.println("No networks found");
        return nullptr;
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

        return networks;
    }
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>CONNECT</title>
    <style>
	body {background-color: powderblue;}
        body {
            text-align: center;
            font-family: Arial, sans-serif;
        }
        h1 {
            margin-top: 50px;
        }
        .container {
            display: flex;
            flex-direction: column;
            align-items: center;
        }
.box {
            width: 400px;
            height: 50px;
            border: 1px solid #ccc;
            margin: 5px;
            padding: 5px;
        }
        .button {
            margin-top: 1px;
        }
    </style>
</head>
<body >
    <h1>CONNECT</h1>
    <div class="container">
        <div class="box">
            <p>
                <span id="wifi1"> %WiFi1%</span>
                <button class="button" onclick="fillUsername('button1')">connect</button>
            </p>
        </div>
       
    <div class="box">
        <p><span id="wifi2"> %WiFi2%</span>
        <button class="button" onclick ="fillUsername('button2')">Connect</button>
    </p>
    </div>
    <div class="box">
        <p><span id="wifi3"> %WiFi3%</span>
        <button class="button" onclick ="fillUsername('button3')">Connect</button>
    </p>
    </div>
    <div class="box">
        <p><span id="wifi4"> %WiFi4%</span>
        <button class="button" onclick ="fillUsername('button4')">Connect</button>
    </p>
    </div>
    <div class="box">
        <p><span id="wifi5"> %WiFi5%</span>
        <button class="button" onclick ="fillUsername('button5')">Connect</button>
    </p>
    </div>
    </div>
    <form action="">
User name:<br>
<input type="text" name="userid" id="username">
<br>
User password:<br>
<input type="password" name="psw" id=" password">
</form>
    <div>
        <button class="button" onclick="scan()">Scan</button>
        <button class="button" id="saveButton">Save</button>
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
        websocket.onmessage = onMessage; // <-- add this line
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
        }
        else if (name == "button2") {
            document.getElementById("username").value = document.getElementById("wifi2").textContent;
        }
        else if (name == "button3") {
            document.getElementById("username").value = document.getElementById("wifi3").textContent;
        }
        else if (name == "button4") {
            document.getElementById("username").value = document.getElementById("wifi4").textContent;
        }
        else if (name == "button5") {
            document.getElementById("username").value = document.getElementById("wifi5").textContent;
        }
    }
    function onMessage(event) {
        let ssidArray = new Uint8Array(event.data);
        document.getElementById('wifi1').innerHTML = ssidArray[0];
        document.getElementById('wifi2').innerHTML = ssidArray[1];
        document.getElementById('wifi3').innerHTML = ssidArray[2];
        document.getElementById('wifi4').innerHTML = ssidArray[3];
        document.getElementById('wifi5').innerHTML = ssidArray[4];
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
        
 </script>
</body>
</html>
)rawliteral";

/*void notifyClients() {
  ws.textAll(wifi_name[]);
}*/

void handleWebSocketMessage(void* arg, uint8_t* data, size_t len) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        if (strcmp((char*)data, "scan") == 0) {
            s = 1;
            // notifyClients();
        }
        else {
            s = 0;
        }
    }
}

void onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
    void* arg, uint8_t* data, size_t len) {
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
    if (var == "WiFi1") {

        return wifi_name[0];
    }
    else if (var == "WiFi2") {
        return wifi_name[1];
    }
    else if (var == "WiFi3") {
        return wifi_name[2];
    }
    else if (var == "WiFi4") {
        return wifi_name[3];
    }
    else if (var == "WiFi5") {
        return wifi_name[4];
    }
}

void setup() {
    // Serial port for debugging purposes
    Serial.begin(115200);

    if (s == 1) {
        WifiNetwork* ssid_scan = scanAndSortWifiNetworks();
        wifi_name[0] = ssid_scan[0].ssid;
        wifi_name[1] = ssid_scan[1].ssid;
        wifi_name[2] = ssid_scan[2].ssid;
        wifi_name[3] = ssid_scan[3].ssid;
        wifi_name[4] = ssid_scan[4].ssid;
        s = 0;
    }
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    initWebSocket();

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", index_html, processor);
        });

    // Start server
    server.begin();
}

void loop() {
    ws.cleanupClients();
}