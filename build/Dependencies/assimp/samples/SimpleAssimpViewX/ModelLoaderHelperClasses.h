#ifndef MESH_HELPER_H
#define MESH_HELPER_H

#include <glad/glad.h>  // 使用 GLAD 替代 OpenGL/OpenGL.h
#include <vector>
#include <assimp/types.h> // 確保能讀到 aiVector3D 等類型

// 對齊 Vertex 結構
#pragma pack(push, 1)
struct Vertex {
    aiVector3D position;
    aiVector3D normal;
    aiColor4D  diffuseColor;
    aiVector3D tangent;
    aiVector3D bitangent;
    aiVector3D uv0;
    aiVector3D uv1;
    GLuint boneIndices[4];
    float  boneWeights[4];
};
#pragma pack(pop)

class MeshHelper {
public:
    // GPU Resources
    GLuint vao = 0;
    GLuint vertexBuffer = 0;
    GLuint indexBuffer = 0;
    GLuint numIndices = 0;
    GLuint textureID = 0;

    // Material
    aiColor4D diffuseColor;
    aiColor4D specularColor;
    aiColor4D ambientColor;
    aiColor4D emissiveColor;

    float opacity = 1.0f;
    float shininess = 0.0f;
    float specularStrength = 0.0f;
    bool  twoSided = false;

    // 初始化與釋放
    MeshHelper();
    ~MeshHelper();

    void setupMeshWithVertices(Vertex* vertices, unsigned int vertexCount,
        unsigned int* indices, unsigned int indexCount);
    void drawMesh();
    void deleteMesh();
};

#endif