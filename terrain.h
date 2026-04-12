#ifndef TERRAIN_H
#define TERRAIN_H

float getVoxelHeight(float x, float z);
float getSceneHeight(float x, float z);
bool isRoad(float x, float z);
bool isRunway(float x, float z);
void getPotentialBuildingRoot(float x, float z, float& rx, float& rz);
bool doesBuildingRootSpawn(float rx, float rz);
void drawVoxelTerrain();
void drawCube(float x, float y, float z,
              float w, float h, float d,
              float r, float g, float b);

#endif
