#ifndef SKY_H
#define SKY_H

struct WeatherProfile {
    float cloudiness;
    float haze;
    float storm;
    float sunBoost;
    float starVisibility;
    float fogStart;
    float fogEnd;
    float skyMute;
};

WeatherProfile getWeatherProfile();
void drawSky(const WeatherProfile& weather);

#endif
