#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <assimp/scene.h>
#include "Mesh.h"

bool loadFBXModel(
    const std::string& path,
    std::vector<Mesh>& meshes,
    std::unordered_map<int, GLuint>& materialTextures
);