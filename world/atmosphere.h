#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

#include "sky.h"

void setupSkyClearColor(const WeatherProfile& weather);
void setupAtmosphericFog(const WeatherProfile& weather);
void setupAtmosphericLighting(const WeatherProfile& weather);
void getSunDirection(float& sx, float& sy, float& sz);
void getActiveLightDirection(float& dx, float& dy, float& dz, bool& isSun);

// Added: Exported for sky rendering pass
void drawSun(float sunX, float sunY, float sunZ, 
             float elevation, const WeatherProfile& weather);

void getSkyColor(float& r, float& g, float& b);
#endif