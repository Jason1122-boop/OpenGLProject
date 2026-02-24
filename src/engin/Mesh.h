#pragma once
#include <glad/glad.h>

struct Mesh
{
    GLuint vao = 0;
    GLuint vbo = 0;
    int vertexCount = 0;
    int materialID = -1;
};