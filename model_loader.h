#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <GL/glut.h>
#include <vector>
#include <string>
#include <memory>

struct LoadedModel {
    std::shared_ptr<Assimp::Importer> importer;  // Keep importer alive - it owns the scene memory!
    const aiScene* scene;
    std::vector<GLuint> textures;
    float scale;
    float offsetX, offsetY, offsetZ;
    aiVector3D minBound, maxBound;
    
    LoadedModel() : importer(nullptr), scene(nullptr), scale(1.0f), 
                    offsetX(0), offsetY(0), offsetZ(0) {}
};

// Load a GLB model file
bool loadModelFromFile(const std::string& filepath, LoadedModel& model);

// Draw the loaded model
void drawLoadedModel(const LoadedModel& model);

// Calculate bounding box of model
void calculateModelBounds(const LoadedModel& model);

// Get list of available plane models in planes/ folder
void getAvailablePlanes(std::vector<std::string>& planeList);

// Clean up model resources
void unloadModel(LoadedModel& model);

#endif
