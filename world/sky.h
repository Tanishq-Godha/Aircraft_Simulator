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

struct CloudLayerConfig {
    float chunkSize;
    int chunkRadius;
    float windXBase;
    float windXStorm;
    float windZBase;
    float windZStorm;
    float minHeightBase;
    float minHeightHaze;
    float heightRange;
    float baseRadius;
    float radiusJitter;
    float baseShade;
    float stormShade;
    float alpha;
    int minClusters;
    int extraClusters;
    int minPuffs;
    int extraPuffs;
    float seedOffset;
};

WeatherProfile getWeatherProfile();
void drawSky(const WeatherProfile& weather);
void initSky();

#endif
