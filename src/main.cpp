#include <Arduino.h>
#include <ArduinoJson.h>
#include <CommonConfig.h>
#include <MqttClient.h>
#include <FotaClient.h>
#include <ESPWifiClient.h>
#include <RemotePrint.h>
#include "settings.h"


MqttClient *mqttClient    = NULL;
FotaClient *fotaClient    = new FotaClient(DEVICE_NAME);
ESPWifiClient *wifiClient = new ESPWifiClient(WIFI_SSID, WIFI_PASS);

long   lastStatusMsgSentAt = 0;
String topics[BLINDS_COUNT];

void getAllTopics(String action, String topics[]) {
  for (int i = 0; i < BLINDS_COUNT; i++) {
    SomfyBlind *blind = &blinds[i];
    String tmp;
    tmp       = action + blind->mqttTopic;
    topics[i] = tmp;
  }
}

SomfyBlind* getBlindFromTopic(String topic) {
  String *lowerTopic = new String(topic.c_str());

  lowerTopic->toLowerCase();

  for (int i = 0; i < BLINDS_COUNT; i++) {
    String *lowerTopicNext = new String(blinds[i].mqttTopic.c_str());
    lowerTopicNext->toLowerCase();

    if (*lowerTopic == *lowerTopicNext) {
      return &blinds[i];
    }
    PRINT("BLIND: Could not find Blind with topic: ");
    PRINTLN(topic);
    return NULL;
  }
}

void publishStatus(bool forcePublish = false,
                   const char *messageId = NULL, String topic = "") {
  long now = millis();

  if ((forcePublish) or (now - lastStatusMsgSentAt >
                         MQTT_PUBLISH_STATUS_INTERVAL)) {
    int count = BLINDS_COUNT;

    if (topic.length()) {
      count = 1;
    }

    for (int i = 0; i < count; i++) {
      SomfyBlind *blind = NULL;

      if (topic.length()) {
        blind = getBlindFromTopic(topic);
      } else {
        blind = &blinds[i];
      }

      const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3);
      DynamicJsonBuffer jsonBuffer(bufferSize);
      JsonObject& root   = jsonBuffer.createObject();
      JsonObject& status = root.createNestedObject("status");

      if (messageId != NULL) {
        root["messageId"] = messageId;
      }

      root["name"]      = blind->name;
      status["powerOn"] = blind->blindPowerOn;

      // convert to String
      String outString;
      root.printTo(outString);

      // publish the message
      String topic = String("get");
      topic.concat(blind->mqttTopic);
      mqttClient->publish(topic, outString);
    }
    lastStatusMsgSentAt = now;
  }
}

void sendToBlind(SomfyBlind *blind, String payload) {
  PRINTLN("BLIND: Send to blind '" + blind->name + "''");

  const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3) + 50;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root   = jsonBuffer.parseObject(payload);
  JsonObject& status = root.get<JsonObject&>("status");

  if (!status.success()) {
    PRINTLN_E("BLIND: JSON with \"status\" key not received.");
    PRINTLN_E(payload);
    return;
  }

  const char *powerOn = status.get<const char *>("powerOn");

  PRINT("BLIND: Power On: ");
  PRINTLN(powerOn);

  // Publish the status here to have quck feedback
  const char *messageId = root.get<const char *>("messageId");

  if (powerOn) {
    if (strcasecmp(powerOn, "true") == 0) {
      blind->blindPowerOn = true;
      publishStatus(true, messageId, blind->mqttTopic);
      blind->remoteButtonUp();
    } else {
      blind->blindPowerOn = false;
      publishStatus(true, messageId, blind->mqttTopic);
      blind->remoteButtonDown();
    }
  }
  const char *action = status.get<const char *>("action");

  if (action) {
    if (strcasecmp(action, "up") == 0) {
      blind->blindPowerOn = true;
      blind->remoteButtonUp();
    } else if (strcasecmp(action, "down") == 0) {
      blind->blindPowerOn = false;
      blind->remoteButtonDown();
    } else if (strcasecmp(action, "stop") == 0) {
      blind->remoteButtonStop();
    } else if (strcasecmp(action, "prog") == 0) {
      blind->remoteButtonProgram();
    } else {
      PRINT("BLIND: Unknown action '");
      PRINT(action);
      PRINTLN("'");
    }
    publishStatus(true, messageId, blind->mqttTopic);
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  PRINT("MQTT Message arrived [");
  PRINT(topic);
  PRINTLN("] ");

  String payloadString = String((char *)payload);
  char  *ptr           = strchr(topic, '/');

  if (ptr != NULL) {
    String lowerTopic = String(ptr);
    lowerTopic.toLowerCase();
    SomfyBlind *blind = getBlindFromTopic(lowerTopic);

    if (blind == NULL) {
      // Blind not found
      return;
    }
    sendToBlind(blind, payloadString);
  }
}

void manualInitialConfiguration() {
  SomfyBlind *blind = NULL;

  blind                       = &blinds[0];
  blind->remoteControllSerial = 0x111000;
  blind->rollingCode          = 10;
  PRINTLN("rollingCode: " + String(blind->rollingCode));
  PRINTLN("remoteControllSerial: " + String(blind->remoteControllSerial));
  blind->save();

  // ========================

  // blind                       = &blinds[1];
  // blind->remoteControllSerial = 0x111001;
  // blind->rollingCode          = 10;
  // PRINTLN("rollingCode: " + String(blind->rollingCode));
  // PRINTLN("remoteControllSerial: " + String(blind->remoteControllSerial));
  // blind->save();

  // ========================
  // blind                       = &blinds[2];
  // blind->remoteControllSerial = 0x111002;
  // blind->rollingCode          = 10;
  // PRINTLN("rollingCode: " + String(blind->rollingCode));
  // PRINTLN("remoteControllSerial: " + String(blind->remoteControllSerial));
  // blind->save();

  for (int i = 0; i < BLINDS_COUNT; i++) {
    SomfyBlind *blind = &blinds[i];
    blind->load();
    PRINTLN("rollingCode: " + String(blind->rollingCode));
    PRINTLN("remoteControllSerial: " + String(blind->remoteControllSerial));
  }
}

void setup() {
  wifiClient->init();
  getAllTopics("set", topics);
  mqttClient = new MqttClient(MQTT_SERVER,
                              MQTT_SERVER_PORT,
                              DEVICE_NAME,
                              MQTT_USERNAME,
                              MQTT_PASS,
                              topics,
                              BLINDS_COUNT,
                              MQTT_SERVER_FINGERPRINT,
                              mqttCallback);
  fotaClient->init();

  // manualInitialConfiguration();
}

void loop() {
  RemotePrint::instance()->handle();
  fotaClient->loop();
  mqttClient->loop();
  publishStatus();
}
