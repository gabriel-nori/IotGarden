#ifndef MQTTCREDENTIAL_H
#define MQTTCREDENTIAL_H

class MqttCredential{
private:

public:
	const char* mqttServer = "YOUR-HOST-HERE";  //Either DNS or IP
	const char* mqttUser = "YOUR-USER"; 
	const char* mqttPassword = "YOUR-PASS";
	const char* outTopic = "iotGarden/"; //This is the default out topic base, but has no problem to change it
	const char* inTopic = "iotGarden/cmd"; //This is the default command topic, but has no problem to change it
	const int mqttPort = 1883; //Default port. Change it as needed
};
#endif
