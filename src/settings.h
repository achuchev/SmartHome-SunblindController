#ifndef SETTING_H
#define SETTING_H

#include "SomfyBlind.h"

#define DEVICE_NAME "SunblindsController"
#define RF_PIN D4
#define BLINDS_COUNT 3

SomfyBlind *smallBlind1 = new SomfyBlind("smallBedroomSunblind1",
                                         "/apartment/smallBedroom/sunblind/1",
                                         RF_PIN);

SomfyBlind *largeBlind1 = new SomfyBlind("largeBedroomSunblind1",
                                         "/apartment/largeBedroom/sunblind/1",
                                         RF_PIN);

SomfyBlind *largeBlind2 = new SomfyBlind("largeBedroomSunblind2",
                                         "/apartment/largeBedroom/sunblind/2",
                                         RF_PIN);
SomfyBlind blinds[BLINDS_COUNT] = { *smallBlind1, *largeBlind1, *largeBlind2 };

#endif // ifndef SETTING_H
