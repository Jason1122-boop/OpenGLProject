#pragma once
#include <string>
#include <glm/glm.hpp>

class Shader
{
public:
    unsigned int ID;

    Shader(const char* vertexSrc, const char* fragmentSrc);

    void use();

    void setMat4(const std::string& name, const glm::mat4& mat);
    void setInt(const std::string& name, int value);
};