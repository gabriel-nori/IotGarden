#include "./WifiCredential.h"
#include <Arduino.h>
#include <PubSubClient.h> //Library to handle MQTT
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define plant1Sensor 4
#define plant2Sensor 5
#define plant3Sensor 6
#define plant4Sensor 7
#define rainSensor 1
#define dhtSensor 3
#define led 2
#define waterTankSenor A0

#define DHTTYPE DHT11
DHT dht(dhtSensor, DHTTYPE);

long otaTimer = 0;

String chipName = "IoTGarden";

void setup() {
	WifiCredential credential;
	Serial.begin(115200);
	delay(10);
	Serial.println("\n\n\n\n\n");
	Serial.println("Booting");
	Serial.println("Connecting to Wi-Fi: SSID:" + String(credential.wifiSsid) + " with password: " + String(credential.wifiPassword));
	WiFi.mode(WIFI_STA);
	WiFi.begin(credential.wifiSsid, credential.wifiPassword);
	long startWifi = millis();

	while(WiFi.status() != WL_CONNECTED){
		if(millis() - startWifi < 50000){
			Serial.print(".");
			delay(100);
		}
		else{
			Serial.println("\nError while connecting to WiFi...");
			ESP.restart();
		}
	}

	Serial.println("WiFiConnected");

	delay(100);

	ArduinoOTA.setHostname(chipName.c_str());
	ArduinoOTA.setPort(3232);

	ArduinoOTA.onStart([]() {
		String type;
		if (ArduinoOTA.getCommand() == U_FLASH){
			type = "sketch";
		}
		else{ // U_SPIFFS
			type = "filesystem";
		}

		// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
		Serial.println("Start updating " + type);
		otaTimer = millis();
	});
	ArduinoOTA.onEnd([]() {
		long otaTime = millis() - otaTimer;
		Serial.println(F("\n\n\n-------------------------------------------------------------------------------------------------"));
		Serial.println("OTA firmware update completed in " + String(otaTime) + " ms, or " + String(otaTime/1000) + " seconds");
		Serial.println(F("-------------------------------------------------------------------------------------------------\n\n\n"));
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
		else if (error == OTA_END_ERROR) Serial.println("End Failed");
	});
  	ArduinoOTA.begin();

	dht.begin();
	Serial.println("Ready");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
	pinMode(led, OUTPUT);
	pinMode(plant1Sensor, INPUT);
	pinMode(plant2Sensor, INPUT);
	pinMode(plant3Sensor, INPUT);
	pinMode(plant4Sensor, INPUT);
	pinMode(rainSensor, INPUT);
	pinMode(waterTankSenor, INPUT);
	delay(10);
	digitalWrite(led, LOW);
}

void loop() {
	ArduinoOTA.handle();
	float humidity = dht.readHumidity();
	float temperature = dht.readTemperature();
	float heatIndex = dht.computeHeatIndex(t, h, false);
}