#include <Arduino.h>
#include <ArtnetWiFi.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ArduinoNvs.h>
#include "index.h"
#include "setup.h"
#include "functions.h"
#include "MillisTimer.h"

bool allowed = 1;
int duration = 0;
int del = 0;

ArtnetWifi artnet;
uint32_t universe1 = 1;
uint32_t universe2;

int address = 1;

int timemultiplier = 118; // This times delay or duration == time on or off
WebServer server(80);

void fogOff() {
    Serial.println("Fog Off");
    digitalWrite(blowPin, LOW);
}

void fogOn() {
    Serial.println("Fog On");
    digitalWrite(blowPin, HIGH);
}

void callback(uint8_t* data, uint16_t size) {
    // Use pre-defined callbacks
}

void handleADC() {
    int a = GetTemp(analogRead(THERMISTORPIN));
    String adcVAlue = String(a);

    server.send(200, "text/plain", adcVAlue);
}

void handleSSID() {
    String adcValue = NVS.getString("ss");
    Serial.println("Got SSID: " + adcValue);

    server.send(200, "text/plain", adcValue);
}

void handlePw() {
    String adcValue = NVS.getString("pw");
    server.send(200, "text/plain", adcValue);
}

void handleDur() {
    int val = NVS.getInt("dur");
    String adcValue = String(val);
    duration = val;
    server.send(200, "text/plain", adcValue);
}

void handleDel() {
    int val = NVS.getInt("del");
    String adcValue = String(val);
    del = val;
    server.send(200, "text/plain", adcValue);
}

void handleUni() {
    String adcValue = NVS.getString("uni");
    server.send(200, "text/plain", adcValue);
}

void handleAdr() {
    int val = NVS.getInt("adr");
    String adcValue = String(val);
    server.send(200, "text/plain", adcValue);
}

void handleRoot() {
    String s = MAIN_page;
    server.send(200, "text/html", s);
}

void setupPage() {
    String s = SETUP_page;
    server.send(200, "text/html", s);
}

void handleSave() {
    bool ok;
    Serial.println("Saving...");
    if (server.arg("adr") != "") {
        Serial.println("Address: " + server.arg("adr"));
        String ad = server.arg("adr");
        int ad2 = ad.toInt();
        ok = NVS.setInt("adr", ad2);
        Serial.println("OK: " + ok);
    }

    if (server.arg("ss") != "") {
        Serial.println("SSID: " + server.arg("ss"));
        ok = NVS.setString ("ss", server.arg("ss"));
        Serial.println("OK: " + ok);
    }

    if (server.arg("pw") != "") {

        Serial.println("PW: " + server.arg("pw"));
        ok = NVS.setString ("pw", server.arg("pw"));
        Serial.println("OK: " + ok);
    }
    if (server.arg("uni") != "") {

        Serial.println("uni: " + server.arg("uni"));
        ok = NVS.setString ("uni", server.arg("uni"));
        Serial.println("OK: " + ok);
    }
    if (server.arg("dur") != "") {
        String ad = server.arg("dur");
        int ad2 = ad.toInt();
        Serial.println("dur: " + server.arg("dur"));
        ok = NVS.setInt ("dur", ad2);
        Serial.println("OK: " + ok);
    }
    if (server.arg("del") != "") {
        String ad = server.arg("del");
        int ad2 = ad.toInt();
        Serial.println("del: " + server.arg("del"));
        ok = NVS.setInt ("del", ad2);
        Serial.println("OK: " + ok);
    }
}

void saveDur(int dur) {
    NVS.setInt("dur", dur);
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data) {
    bool tail = false;

    Serial.print("DMX: Univ: ");
    Serial.print(universe, DEC);
    Serial.print(", Seq: ");
    Serial.print(sequence, DEC);
    Serial.print(", Data (");
    Serial.print(length, DEC);
    Serial.print("): ");

    if (length > 16) {
        length = 16;
        tail = true;
    }

    // send out the buffer
    for (uint16_t i = 0; i < length; i++) {
        if (i == address) {
            NVS.setInt("dur", data[i]);
        }
        if (i == address + 1) {
            NVS.setInt("del", data[i]);
        }
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    if (tail) {
        Serial.print("...");
    }
    Serial.println();
}

bool connectWifi(void) {
    bool state = true;
    int i = 0;

    WiFi.begin(ssid, password);
    Serial.println("");
    Serial.println("Connecting to WiFi...");

    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if (i > 20) {
            state = false;
            break;
        }
        i++;
    }
    if (state) {
        Serial.println("");
        Serial.print("Connected to ");
        Serial.println(ssid);
        Serial.print("IP Address: ");
        Serial.println(IPAddress(WiFi.localIP()));
    } else {
        Serial.println("");
        Serial.println("Connection failed.");
    }

    return state;
}

void setup(void) {
    NVS.begin();
    pinMode(heatPin, OUTPUT);
    pinMode(blowPin, OUTPUT);

    Serial.begin(115200);

    connectWifi();

    Serial.println("Booting...");

    address = 1;

    artnet.setArtDmxCallback(onDmxFrame);
    artnet.begin();

    Serial.println("Ready.");
    server.on("/", handleRoot);      //This is display page
    server.on("/readADC", handleADC);//To get update of ADC Value only
    server.on("/setup", setupPage);//To get update of ADC Value only
    server.on("/readUNI", handleUni);//To get update of Universe Value only
    server.on("/readADR", handleAdr);//To get update of Starting Address only
    server.on("/readSS", handleSSID);//To get update of SSID only
    server.on("/readPW", handlePw);//To get update of WiFi PW only
    server.on("/readDUR", handleDur);//To get update of Duration only
    server.on("/readDEL", handleDel);//To get update of Duration only

    server.on("/save", handleSave);
    server.begin();
    Serial.println("HTTP server started");

    setupOta();
    int c = (1000 * NVS.getInt("dur")) / 2;
    Serial.print(" x=: ");
    Serial.println(c);
}

unsigned long previousMillis = 0;
unsigned long previousMillis2 = 0;

void runFog() {
    unsigned long currentMillis = millis();
    unsigned long currentMillis2 = millis();

    if (allowed == 1) {
        if (currentMillis - previousMillis >= 100 * NVS.getInt("del")) {
            // Save the last time LED blinked
            previousMillis = currentMillis;
            //Serial.println("TEST");
            fogOn();
            allowed = 0;
        }
    }

    if (currentMillis2 - previousMillis2 >= 100 * NVS.getInt("dur")) {
        // Save the last time LED blinked
        previousMillis2 = currentMillis2;
        fogOff();
        allowed = 1;
    }
}

void loop() {
    int t = avg();
    Serial.println(t);
    if (t <= 300 && t > 0) {
        digitalWrite(heatPin, HIGH);
        runFog();
    } else if (t < 0) {
        digitalWrite(heatPin, LOW);
        digitalWrite(blowPin, LOW);
        Serial.println("FOG OFF: Under Temp");
    } else {
        digitalWrite(heatPin, LOW);
        digitalWrite(blowPin, LOW);
        Serial.println("FOG OFF: Over Temp");
    }

    ArduinoOTA.handle();
    server.handleClient();
    artnet.read();

    delay(300);
}