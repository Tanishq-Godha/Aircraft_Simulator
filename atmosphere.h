#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

#include "sky.h"

void setupSkyClearColor(const WeatherProfile& weather);
void setupAtmosphericFog(const WeatherProfile& weather);
void setupAtmosphericLighting(const WeatherProfile& weather);

#endif
