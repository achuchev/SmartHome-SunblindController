#ifndef SOMFY_BLIND_H
#define SOMFY_BLIND_H

#include <Arduino.h>
#include "SomfyRemote.h"

class SomfyBlind {
public:

  SomfyBlind();
  SomfyBlind(String name,
             String mqttTopic,
             int    rfPin);
  void        setRemoteControllSerial(int remoteControllSerial);
  void        setRollingCode(uint16_t rollingCode);
  void        remoteButtonUp();
  void        remoteButtonDown();
  void        remoteButtonStop();
  void        remoteButtonProgram();
  bool        load();
  bool        save();
  SomfyRemote remote();

  String mqttTopic;
  String name;
  int rfPin;
  int remoteControllSerial;
  uint16_t rollingCode;
  bool blindPowerOn;

private:

  void remoteButton(byte button);
};

#endif // ifndef SOMFY_BLIND_h
