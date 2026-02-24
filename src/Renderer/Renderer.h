#pragma once
#include "Mesh.h"
#include "Shader.h"
#include <vector>
#include <unordered_map>

class Renderer
{
public:
    void drawMeshes(
        const std::vector<Mesh>& meshes,
        std::unordered_map<int, unsigned int>& textures,
        Shader& shader
    );
};