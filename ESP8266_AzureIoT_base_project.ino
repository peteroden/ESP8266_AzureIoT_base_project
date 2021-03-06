// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <SPI.h>

#include <ESP8266WiFi.h>

#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
#include <ArduinoOTA.h>

#include <AzureIoTHub.h>
#include <AzureIoTHubClient.h>
#include <AzureIoTUtility.h>
#include <AzureIoTProtocol_MQTT.h>
#include <AzureIoTProtocol_HTTP.h>

//include one or more of the sensors below by uncommenting the corresponding #define statement
//#define DUMMYSENSOR
//#define DHT11SENSOR
//#define LIGHTSENSOR
//#define PIRSENSOR
//#define SWITCHSENSOR

/*String containing Hostname, Device Id & Device Key in the format:             */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"    */
static const char* connectionString = "HostName=MQTTIoT.azure-devices.net;DeviceId=WeMos;SharedAccessKey=ypn7H369RPpYEuhmwAnOLs1FGxMoNh+uaMyJr+KgnHs=";

//then include the sensor code
#include "iotsensors.h"

WiFiClientSecure sslClient; // for ESP8266
AzureIoTHubClient iotHubClient;
IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;


void setup() {    
    initSerial();
    initWifi();
    initTime();

    initAzureIoT(HTTP_Protocol);
    initIotSensors();
}

void loop() {
  updateSensors();
}

void initSerial() {
    // Start serial and initialize stdout
    Serial.begin(115200);
    Serial.setDebugOutput(true);
}

void initWifi() {

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();

        //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("AutoConnectAP");
    //or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();

    Serial.println("Connected to wifi");
}

void initTime() {
    time_t epochTime;
    int retries = 0;
    
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    while (retries < 30) {
        epochTime = time(NULL);

        if (epochTime == 0) {
            Serial.println("Fetching NTP epoch time failed! Waiting 2 seconds to retry.");
            delay(2000);
            retries++;
        } else {
            Serial.print("Fetched NTP epoch time is: ");
            Serial.println(epochTime);
            break;
        }
    }
}

void initAzureIoT(IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol) {
    iotHubClient.begin(sslClient);
    serializer_init(NULL);
    iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, protocol);
    if (iotHubClientHandle == NULL) {
      Serial.println("couldn't connect to Azure IoT Hub");
    } else {
      const char* message = "device connected";
      IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, receiveAzureIoTMessage, NULL);
      //IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, receiveAzureIoTMessage, myWeather);
      sendAzureIoTMessage(iotHubClientHandle, (const unsigned char*)message, sizeof(message));
    }  
}


