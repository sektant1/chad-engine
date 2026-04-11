#include <engine/renderer/model.h>
#include <engine/core/log.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glad/glad.h>
#include <stb_image.h>

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace chad
{

// ============================================================================
// Helpers
// ============================================================================

static std::string directoryOf(const char *path)
{
    std::string s(path);
    auto        pos = s.find_last_of("/\\");
    if (pos != std::string::npos) {
        return s.substr(0, pos + 1);
    }
    return "./";
}

static Texture loadEmbeddedTexture(const aiTexture *ai_tex)
{
    Texture tex = {};

    // Compressed texture (e.g. PNG/JPG stored in memory)
    if (ai_tex->mHeight == 0) {
        int            w    = 0;
        int            h    = 0;
        int            ch   = 0;
        unsigned char *data = stbi_load_from_memory(
            reinterpret_cast<const unsigned char *>(ai_tex->pcData),
            static_cast<int>(ai_tex->mWidth),
            &w,
            &h,
            &ch,
            4);

        if (data == nullptr) {
            LOG_WARN("Failed to decode embedded texture");
            return tex;
        }

        glGenTextures(1, &tex.id);
        glBindTexture(GL_TEXTURE_2D, tex.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        tex.width    = w;
        tex.height   = h;
        tex.channels = ch;
        stbi_image_free(data);
    } else {
        LOG_WARN("Embedded raw (uncompressed) texture not supported yet");
    }

    return tex;
}

static Mesh processMesh(const aiMesh *ai_mesh)
{
    std::vector<Vertex> vertices(ai_mesh->mNumVertices);
    std::vector<u32>    indices;

    for (u32 idx = 0; idx < ai_mesh->mNumVertices; idx++) {
        Vertex &vertex = vertices[idx];

        vertex.position[0] = ai_mesh->mVertices[idx].x;
        vertex.position[1] = ai_mesh->mVertices[idx].y;
        vertex.position[2] = ai_mesh->mVertices[idx].z;

        if (ai_mesh->mTextureCoords[0] != nullptr) {
            vertex.texcoord[0] = ai_mesh->mTextureCoords[0][idx].x;
            vertex.texcoord[1] = ai_mesh->mTextureCoords[0][idx].y;
        } else {
            vertex.texcoord[0] = 0.0F;
            vertex.texcoord[1] = 0.0F;
        }

        if (ai_mesh->mNormals != nullptr) {
            vertex.normal[0] = ai_mesh->mNormals[idx].x;
            vertex.normal[1] = ai_mesh->mNormals[idx].y;
            vertex.normal[2] = ai_mesh->mNormals[idx].z;
        } else {
            vertex.normal[0] = 0.0F;
            vertex.normal[1] = 1.0F;
            vertex.normal[2] = 0.0F;
        }

        if (ai_mesh->mColors[0] != nullptr) {
            vertex.color[0] = ai_mesh->mColors[0][idx].r;
            vertex.color[1] = ai_mesh->mColors[0][idx].g;
            vertex.color[2] = ai_mesh->mColors[0][idx].b;
            vertex.color[3] = ai_mesh->mColors[0][idx].a;
        } else {
            vertex.color[0] = 1.0F;
            vertex.color[1] = 1.0F;
            vertex.color[2] = 1.0F;
            vertex.color[3] = 1.0F;
        }
    }

    for (u32 i = 0; i < ai_mesh->mNumFaces; i++) {
        const aiFace &face = ai_mesh->mFaces[i];
        for (u32 j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    return meshCreate(
        vertices.data(),
        static_cast<u32>(vertices.size()),
        indices.data(),
        static_cast<u32>(indices.size()));
}

// ============================================================================
// Public API
// ============================================================================

Model modelLoad(const char *path)
{
    Model model = {};

    Assimp::Importer importer;
    const aiScene   *scene = importer.ReadFile(
        path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices);

    if ((scene == nullptr) || ((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0U) || (scene->mRootNode == nullptr)) {
        LOG_ERROR("Assimp: %s", importer.GetErrorString());
        return model;
    }

    std::string dir = directoryOf(path);

    for (u32 i = 0; i < scene->mNumMeshes && model.mesh_count < MODEL_MAX_MESHES; i++) {
        ModelMesh &model_mesh  = model.meshes[model.mesh_count];
        model_mesh.mesh        = processMesh(scene->mMeshes[i]);
        model_mesh.has_texture = false;
        model_mesh.texture     = {};

        // Try to load diffuse texture
        u32 mat_idx = scene->mMeshes[i]->mMaterialIndex;
        if (mat_idx < scene->mNumMaterials) {
            aiMaterial *mat = scene->mMaterials[mat_idx];
            aiString    tex_path;

            if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &tex_path) == AI_SUCCESS
                || mat->GetTexture(aiTextureType_BASE_COLOR, 0, &tex_path) == AI_SUCCESS)
            {
                // Try embedded texture first (works for GLB)
                const aiTexture *embedded = scene->GetEmbeddedTexture(tex_path.C_Str());
                if (embedded != nullptr) {
                    model_mesh.texture     = loadEmbeddedTexture(embedded);
                    model_mesh.has_texture = (model_mesh.texture.id != 0);
                } else if (tex_path.data[0] == '*') {
                    int tex_idx = std::atoi(tex_path.data + 1);
                    if (tex_idx >= 0 && static_cast<u32>(tex_idx) < scene->mNumTextures) {
                        model_mesh.texture     = loadEmbeddedTexture(scene->mTextures[tex_idx]);
                        model_mesh.has_texture = (model_mesh.texture.id != 0);
                    }
                } else {
                    // External texture file
                    std::string full_path  = dir + tex_path.C_Str();
                    model_mesh.texture     = textureLoad(full_path.c_str());
                    model_mesh.has_texture = (model_mesh.texture.id != 0);
                }
            }
        }

        model.mesh_count++;
    }

    if (scene->mNumMeshes > MODEL_MAX_MESHES) {
        LOG_WARN("Model %s has %u meshes, truncated to %u", path, scene->mNumMeshes, MODEL_MAX_MESHES);
    }

    LOG_INFO("Model loaded: %s (%u meshes)", path, model.mesh_count);
    return model;
}

void modelDraw(const Model &model)
{
    for (u32 i = 0; i < model.mesh_count; i++) {
        const ModelMesh &model_mesh = model.meshes[i];
        if (model_mesh.has_texture) {
            textureBind(model_mesh.texture, 0);
        }
        meshDraw(model_mesh.mesh);
    }
}

void modelDestroy(Model &model)
{
    for (u32 i = 0; i < model.mesh_count; i++) {
        meshDestroy(model.meshes[i].mesh);
        if (model.meshes[i].has_texture) {
            textureDestroy(model.meshes[i].texture);
        }
    }
    model.mesh_count = 0;
}

}  // namespace chad
