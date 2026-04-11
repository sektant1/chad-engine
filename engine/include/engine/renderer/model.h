#pragma once

#include <engine/core/types.h>
#include <engine/renderer/mesh.h>
#include <engine/renderer/texture.h>

namespace chad
{

static constexpr u32 MODEL_MAX_MESHES = 32;

struct ModelMesh
{
    Mesh    mesh;
    Texture texture;  // diffuse texture (id=0 if none)
    bool    has_texture;
};

struct Model
{
    ModelMesh meshes[MODEL_MAX_MESHES];
    u32       mesh_count;
};

// Load a 3D model file (GLB, GLTF, FBX, OBJ).
// Textures are loaded from embedded data or from the model's directory.
Model modelLoad(const char *path);

// Draw all meshes in the model. Caller must bind shader beforehand.
// Binds each mesh's diffuse texture to slot 0 if available.
void modelDraw(const Model &model);

// Free all GPU resources (meshes + textures).
void modelDestroy(Model &model);

}  // namespace chad
