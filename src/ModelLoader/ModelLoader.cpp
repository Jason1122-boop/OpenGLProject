#include "ModelLoader.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <stb_image.h>
#include <iostream>

//processmesh do it
// 處理單一 Mesh
void processMesh(aiMesh* mesh, const aiScene* scene, std::vector<Mesh>& meshes)
{
    std::vector<float> vertexData;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        // ===== Position =====
        vertexData.push_back(mesh->mVertices[i].x);
        vertexData.push_back(mesh->mVertices[i].y);
        vertexData.push_back(mesh->mVertices[i].z);

        // ===== UV (只取第一層) =====
        if (mesh->mTextureCoords[0])
        {
            vertexData.push_back(mesh->mTextureCoords[0][i].x);
            vertexData.push_back(mesh->mTextureCoords[0][i].y);
        }
        else
        {
            vertexData.push_back(0.0f);
            vertexData.push_back(0.0f);
        }
    }

    Mesh myMesh{};

    glGenVertexArrays(1, &myMesh.vao);
    glGenBuffers(1, &myMesh.vbo);

    glBindVertexArray(myMesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, myMesh.vbo);

    glBufferData(
        GL_ARRAY_BUFFER,
        vertexData.size() * sizeof(float),
        vertexData.data(),
        GL_STATIC_DRAW
    );

    // layout(location = 0) -> position
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        (void*)0
    );
    glEnableVertexAttribArray(0);

    // layout(location = 1) -> UV
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        (void*)(3 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    myMesh.vertexCount = mesh->mNumVertices;
    myMesh.materialID = mesh->mMaterialIndex;

    meshes.push_back(myMesh);
}

// 遞迴處理 FBX 節點樹
void processNode(aiNode* node, const aiScene* scene, std::vector<Mesh>& meshes)
{
    // 處理當前節點的 Mesh
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene, meshes);
    }

    // 遞迴處理子節點
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene, meshes);
    }
}



//fbx讀取
bool loadFBXModel(const std::string& path, std::vector<Mesh>& meshes, std::unordered_map<int, GLuint>& materialTextures) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp Error: " << importer.GetErrorString() << std::endl;
        return false;
    }

    // 1. 取得資料夾與 .fbm 資料夾路徑
    std::string directory = path.substr(0, path.find_last_of("/\\") + 1);
    std::string baseName = path.substr(path.find_last_of("/\\") + 1);
    baseName = baseName.substr(0, baseName.find_last_of("."));

    // 指向 temu2000-2.fbm/
    std::string fbmPath = directory + baseName + ".fbm/";

    // 2. 處理貼圖
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* mat = scene->mMaterials[i];
        aiString str;
        if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &str) == AI_SUCCESS) {
            std::string filename = std::string(str.C_Str());

            // 檔名
            size_t lastSlash = filename.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                filename = filename.substr(lastSlash + 1);
            }

            // 1. 先找 .fbm 資料夾
            std::string texPath = fbmPath + filename;
            int width, height, nrChannels;
            unsigned char* data = stbi_load(texPath.c_str(), &width, &height, &nrChannels, 0);

            // 2. 如果失敗，再找模型同層目錄
            if (!data) {
                texPath = directory + filename;
                data = stbi_load(texPath.c_str(), &width, &height, &nrChannels, 0);
            }

            if (data) {
                std::cout << "貼圖載入成功: " << texPath << std::endl; // 這行很重要，幫你確認路徑
                GLuint textureID;
                glGenTextures(1, &textureID);
                glBindTexture(GL_TEXTURE_2D, textureID);
                GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
                glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);
                stbi_image_free(data);
                materialTextures[i] = textureID;
            }
            else {
                std::cerr << "貼圖載入失敗，找不到檔案: " << filename << std::endl;
            }
        }
    }

    // 3. 處理節點
    processNode(scene->mRootNode, scene, meshes);
    return true;
}