#include "./WifiCredential.h"
#include "./MqttCredential.h"
#include "DHT.h"
#include <Arduino.h>
#include <PubSubClient.h> //Library to handle MQTT
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define plant1Sensor 5
#define plant2Sensor 14
#define plant3Sensor 12
#define plant4Sensor 13
#define rainSensor 10
#define dhtSensor 4
#define led 16
#define waterTankSenor A0

#define DHTTYPE DHT11
DHT dht(dhtSensor, DHTTYPE);

long otaTimer = 0;

String chipName = "IoTGarden";

bool debug = false;

long
	lastMqttTry = 0,
	mqttRetryTime = 0;

MqttCredential mqttCredential;
WiFiClient netClient;
PubSubClient mqttClient(netClient);

const char* mqttServer = mqttCredential.mqttServer;
const char* mqttUser = mqttCredential.mqttUser;
const char* mqttPassword = mqttCredential.mqttPassword;
const char* outTopic = mqttCredential.outTopic;
const char* inTopic = mqttCredential.inTopic;
const int mqttPort = mqttCredential.mqttPort;

void mqttCallBack(char* topic, byte* payload, unsigned int length);


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

	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true); 

	while(WiFi.status() != WL_CONNECTED){
		if(millis() - startWifi < 50000){
			Serial.print(".");
			delay(100);
		}
		else{
			Serial.println("\nError while connecting to WiFi...");
			ESP.restart();
		}
		ESP.wdtFeed();
	}

	Serial.println("WiFiConnected");

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

	Serial.println("Ready");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
	dht.begin();
	ESP.wdtFeed();

	pinMode(led, OUTPUT);
	pinMode(plant1Sensor, INPUT);
	pinMode(plant2Sensor, INPUT);
	pinMode(plant3Sensor, INPUT);
	pinMode(plant4Sensor, INPUT);
	pinMode(rainSensor, INPUT);
	pinMode(waterTankSenor, INPUT);
	digitalWrite(led, HIGH);

	delay(10);

	mqttClient.setServer(mqttServer, mqttPort);
	mqttClient.setCallback(mqttCallBack);
	connectMqtt();
}

void loop() {
	ArduinoOTA.handle();

	if(mqttClient.connected()){
		mqttClient.loop();
	}
	else{
		reconnectMqtt();
	}

	float humidity = dht.readHumidity();
	float temperature = dht.readTemperature();
	float heatIndex = dht.computeHeatIndex(temperature, humidity, false);

	Serial.println(F("Sending frame..."));

	sendMqtt("temperature", String(temperature));
	sendMqtt("humidity", String(humidity));
	sendMqtt("heatindex", String(heatIndex));
	sendMqtt("plant1", String(!digitalRead(plant1Sensor)));
	sendMqtt("plant2", String(!digitalRead(plant2Sensor)));
	sendMqtt("plant3", String(!digitalRead(plant3Sensor)));
	sendMqtt("plant4", String(!digitalRead(plant4Sensor)));
	sendMqtt("rain", String(!digitalRead(rainSensor)));

	digitalWrite(led, LOW);
	delay(100);
	digitalWrite(led, HIGH);
	delay(1000);
}

void mqttCallBack(char* topic, byte* payload, unsigned int length) {
	if(debug){
		Serial.print(F("Message arrived ["));
		Serial.print(topic);
		Serial.print(F("] "));
	}
	String payloadIn;
	for (int i = 0; i < length; i++) {
		payloadIn += (char)payload[i];
	}
	if(debug){
		Serial.print(F("\nPayload from MQTT: " ));
		Serial.println(payloadIn);
	}
	//router(payloadIn);
}

void reconnectMqtt(){
	if(!millis() - lastMqttTry > mqttRetryTime){
		if(debug){
			Serial.println(F("MQTT has gone down... Reconnecting"));
		}
		connectMqtt();
		lastMqttTry = millis();
	}
}

void connectMqtt() {
	if(debug){
		Serial.print(F("Attempting MQTT connection..."));
	}
	// Attempt to connect
	if (mqttClient.connect(chipName.c_str(), mqttUser, mqttPassword)) {
		if(debug){
			Serial.println(F("MQTT connected"));
		}
		// Once connected, publish the connect message and the current switch state
		mqttClient.publish((String(outTopic) + "/stat").c_str(), "connected");
		mqttClient.subscribe(inTopic);
		digitalWrite(led, LOW);
	} else {
		Serial.print(F("failed, rc="));
		Serial.print(mqttClient.state());
		Serial.println(F(" try again in 5 seconds"));
		
	}
}

void sendMqtt(String sensor, String value){
	mqttClient.publish((String(outTopic) + sensor).c_str(), value.c_str());
}