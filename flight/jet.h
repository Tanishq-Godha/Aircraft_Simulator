#ifndef JET_H
#define JET_H

#include "model_loader.h"

extern LoadedModel g_loadedJetModel;
extern bool g_useLoadedModel;

void drawDetailedJet();
void drawJet();
void drawExplosionAndSparks();  // Wrapper that uses loaded model or default

bool loadSelectedPlaneModel(const std::string& planeFilename);
void unloadJetModel();

#endif