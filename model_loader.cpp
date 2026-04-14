#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "model_loader.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <cstdio>
#include <cmath>
#include <memory>
#include <dirent.h>
#include <algorithm>

// GLuint loadEmbeddedTexture: Load texture from assimp
GLuint loadEmbeddedTexture(const aiTexture* tex) {
    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    int w, h, comp;
    unsigned char* data = nullptr;

    // Compressed texture (PNG/JPG)
    if (tex->mHeight == 0) {
        unsigned char* px = (unsigned char*)tex->pcData;
        data = stbi_load_from_memory(
            px,
            tex->mWidth,
            &w, &h, &comp,
            STBI_rgb_alpha
        );
        if (!data) {
            printf("WARNING: STB Image failed to load texture.\n");
            fflush(stdout);
        }
    } else {
        // Uncompressed
        w = tex->mWidth;
        h = tex->mHeight;
        data = (unsigned char*)tex->pcData;
    }

    if (data) {
        GLenum format = (tex->mHeight == 0) ? GL_RGBA : 0x80E1; // GL_BGRA = 0x80E1
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    }

    if (tex->mHeight == 0 && data)
        stbi_image_free(data);

    return id;
}

// Calculate bounding box of a node recursively
void calculateNodeBounds(const aiNode* node, const aiScene* scene,
                        aiVector3D& minBound, aiVector3D& maxBound,
                        const aiMatrix4x4& parentTransform) {
    aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;

    for (unsigned i = 0; i < node->mNumMeshes; i++) {
        const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        
        for (unsigned j = 0; j < mesh->mNumVertices; j++) {
            aiVector3D v = mesh->mVertices[j];
            aiVector3D transformed = nodeTransform * v;

            minBound.x = std::min(minBound.x, transformed.x);
            minBound.y = std::min(minBound.y, transformed.y);
            minBound.z = std::min(minBound.z, transformed.z);
            
            maxBound.x = std::max(maxBound.x, transformed.x);
            maxBound.y = std::max(maxBound.y, transformed.y);
            maxBound.z = std::max(maxBound.z, transformed.z);
        }
    }

    for (unsigned i = 0; i < node->mNumChildren; i++) {
        calculateNodeBounds(node->mChildren[i], scene, minBound, maxBound, nodeTransform);
    }
}

void calculateModelBounds(const LoadedModel& model) {
    if (!model.scene) return;

    aiVector3D minBound(1e10f, 1e10f, 1e10f);
    aiVector3D maxBound(-1e10f, -1e10f, -1e10f);
    aiMatrix4x4 identity;

    calculateNodeBounds(model.scene->mRootNode, model.scene, minBound, maxBound, identity);

    // Const cast required - we know this is safe
    LoadedModel& mutable_model = const_cast<LoadedModel&>(model);
    mutable_model.minBound = minBound;
    mutable_model.maxBound = maxBound;
}

// Apply material settings from assimp - EXACTLY like test code
void applyMaterial(const aiMaterial* mat, const LoadedModel& model) {
    if (!mat) {
        glDisable(GL_TEXTURE_2D);
        return;
    }

    aiString texPath;

    if (mat->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) == AI_SUCCESS ||
        mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
    {
        if (texPath.C_Str()[0] == '*')
        {
            int index = atoi(texPath.C_Str() + 1);

            if (index >= 0 && index < (int)model.textures.size()) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, model.textures[index]);
                return;
            }
        }
    }

    glDisable(GL_TEXTURE_2D);
}

// Draw a single mesh - EXACTLY like test code
void drawMesh(const aiMesh* mesh, const LoadedModel& model) {
    if (!mesh || !model.scene) {
        return;
    }

    if (mesh->mNumFaces == 0 || mesh->mNumVertices == 0) {
        return;
    }

    const aiMaterial* mat = nullptr;
    if (mesh->mMaterialIndex < model.scene->mNumMaterials) {
        mat = model.scene->mMaterials[mesh->mMaterialIndex];
    }
    applyMaterial(mat, model);

    glBegin(GL_TRIANGLES);

    for (unsigned i = 0; i < mesh->mNumFaces; i++)
    {
        const aiFace& f = mesh->mFaces[i];

        for (unsigned j = 0; j < f.mNumIndices; j++)
        {
            int idx = f.mIndices[j];

            if (mesh->HasNormals())
            {
                auto n = mesh->mNormals[idx];
                glNormal3f(n.x, n.y, n.z);
            }

            if (mesh->HasTextureCoords(0))
            {
                auto uv = mesh->mTextureCoords[0][idx];
                glTexCoord2f(uv.x, uv.y);
            }

            auto v = mesh->mVertices[idx];
            glVertex3f(v.x, v.y, v.z);
        }
    }

    glEnd();
}

// Recursively draw node and its children
void drawNode(const aiNode* node, const LoadedModel& model) {
    if (!node || !model.scene) return;

    // Apply node transformation matrix
    float matrix[16];
    const aiMatrix4x4& nodeTransform = node->mTransformation;
    matrix[0] = nodeTransform.a1;  matrix[4] = nodeTransform.a2;  matrix[8]  = nodeTransform.a3;  matrix[12] = nodeTransform.a4;
    matrix[1] = nodeTransform.b1;  matrix[5] = nodeTransform.b2;  matrix[9]  = nodeTransform.b3;  matrix[13] = nodeTransform.b4;
    matrix[2] = nodeTransform.c1;  matrix[6] = nodeTransform.c2;  matrix[10] = nodeTransform.c3;  matrix[14] = nodeTransform.c4;
    matrix[3] = nodeTransform.d1;  matrix[7] = nodeTransform.d2;  matrix[11] = nodeTransform.d3;  matrix[15] = nodeTransform.d4;

    glPushMatrix();
    glMultMatrixf(matrix);

    // Draw meshes in this node
    for (unsigned i = 0; i < node->mNumMeshes; i++) {
        if (node->mMeshes[i] < model.scene->mNumMeshes) {
            drawMesh(model.scene->mMeshes[node->mMeshes[i]], model);
        }
    }

    // Recursively draw child nodes
    for (unsigned i = 0; i < node->mNumChildren; i++) {
        if (node->mChildren[i]) {
            drawNode(node->mChildren[i], model);
        }
    }

    glPopMatrix();
}

bool loadModelFromFile(const std::string& filepath, LoadedModel& model) {
    printf("Loading model: %s\n", filepath.c_str());
    fflush(stdout);
    
    // Create a new importer and keep it alive with the model via shared_ptr
    auto importer = std::make_shared<Assimp::Importer>();
    
    const aiScene* scene = importer->ReadFile(
        filepath,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_FlipUVs
    );

    if (!scene) {
        printf("ERROR loading model: %s\n", importer->GetErrorString());
        fflush(stdout);
        return false;
    }

    if (!scene->mRootNode) {
        printf("ERROR: Scene has no root node\n");
        fflush(stdout);
        return false;
    }

    // Store the importer to keep scene memory alive
    model.importer = importer;
    model.scene = scene;
    
    if (scene->mNumTextures > 0) {
        model.textures.resize(scene->mNumTextures);
        for (unsigned i = 0; i < scene->mNumTextures; i++) {
            if (scene->mTextures[i]) {
                model.textures[i] = loadEmbeddedTexture(scene->mTextures[i]);
            }
        }
    }

    // Calculate bounds
    calculateModelBounds(model);

    // Calculate scale to match default plane size (roughly 16 units long/wide)
    float sizeX = model.maxBound.x - model.minBound.x;
    float sizeY = model.maxBound.y - model.minBound.y;
    float sizeZ = model.maxBound.z - model.minBound.z;
    float maxSize = std::max({sizeX, sizeY, sizeZ});
    
    if (maxSize > 0.01f) {
        model.scale = 16.0f / maxSize;
    } else {
        model.scale = 1.0f;
    }

    // Calculate center offset to place model at origin
    model.offsetX = -(model.minBound.x + sizeX / 2.0f);
    model.offsetY = -(model.minBound.y + sizeY / 2.0f);
    model.offsetZ = -(model.minBound.z + sizeZ / 2.0f);

    printf("Model loaded: Meshes=%d, Textures=%d, Scale=%.2f\n", 
           scene->mNumMeshes, scene->mNumTextures, model.scale);
    fflush(stdout);

    return true;
}

void drawLoadedModel(const LoadedModel& model) {
    if (!model.scene || !model.importer) {
        return;
    }

    if (!model.scene->mRootNode) {
        return;
    }
    
    // Disable lighting to show texture colors
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG); // Ensure fog doesn't overwrite it
    glDisable(GL_BLEND);
    
    // Force texturing mode
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
    // Set white color - this is the default for textures
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    glPushMatrix();

    
    glScalef(model.scale, model.scale, model.scale);
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
    glTranslatef(model.offsetX, model.offsetY, model.offsetZ);
    
    drawNode(model.scene->mRootNode, model);
    
    glPopMatrix();
    
    // Re-enable lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_FOG);
    glEnable(GL_BLEND);
}

void getAvailablePlanes(std::vector<std::string>& planeList) {
    planeList.clear();
    planeList.push_back("DEFAULT");  // Default procedural jet

    DIR* dir = opendir("planes");
    if (!dir) {
        printf("Info: planes/ folder not found, creating it is optional\n");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        
        // Check for .glb or .fbx extensions
        if (name.size() > 4) {
            std::string ext = name.substr(name.size() - 4);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            if (ext == ".glb" || ext == ".fbx") {
                planeList.push_back(name);
            }
        }
    }
    closedir(dir);

    printf("Found %zu available planes\n", planeList.size());
}

void unloadModel(LoadedModel& model) {
    printf("Unloading model...\n");
    fflush(stdout);
    
    if (model.scene || model.importer) {
        // Delete GL textures
        for (GLuint tex : model.textures) {
            if (tex != 0) {
                glDeleteTextures(1, &tex);
            }
        }
        model.textures.clear();
        
        // Release importer (which will free the scene memory)
        model.importer.reset();
        model.scene = nullptr;
        
        printf("Model unloaded successfully.\n");
    }
    fflush(stdout);
}
