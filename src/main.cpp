#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <windows.h>
#include <commdlg.h>
#include <direct.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/material.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "tiny_obj_loader.h"
#include "Camera.h"
#include "ModelLoader.h"
#include "Shader.h"
#include "Mesh.h"
#include "InputManager.h"
#include "Renderer.h"
#include "FileDialog.h"
#include "Texture.h"


struct Mesh {
    GLuint vao;
    GLuint vbo;
    int vertexCount;
    int materialID;
};

// File Dialog
std::string OpenFileDialog()
{
    OPENFILENAMEA ofn{};
    CHAR szFile[MAX_PATH] = { 0 };

    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Model Files\0*.fbx;*.obj\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn))
        return szFile;

    return "";
}

// Mesh Processing
void processMesh(aiMesh* mesh,
    std::vector<Mesh>& meshes)
{
    std::vector<float> vertexData;

    for (unsigned i = 0; i < mesh->mNumVertices; i++)
    {
        vertexData.push_back(mesh->mVertices[i].x);
        vertexData.push_back(mesh->mVertices[i].y);
        vertexData.push_back(mesh->mVertices[i].z);

        if (mesh->mTextureCoords[0])
        {
            vertexData.push_back(mesh->mTextureCoords[0][i].x);
            vertexData.push_back(mesh->mTextureCoords[0][i].y);
        }
        else
        {
            vertexData.push_back(0);
            vertexData.push_back(0);
        }
    }

    Mesh m{};

    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);

    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);

    glBufferData(GL_ARRAY_BUFFER,
        vertexData.size() * sizeof(float),
        vertexData.data(),
        GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    m.vertexCount = mesh->mNumVertices;
    m.materialID = mesh->mMaterialIndex;

    meshes.push_back(m);
}

void processNode(aiNode* node,
    const aiScene* scene,
    std::vector<Mesh>& meshes)
{
    for (unsigned i = 0; i < node->mNumMeshes; i++)
        processMesh(scene->mMeshes[node->mMeshes[i]], meshes);

    for (unsigned i = 0; i < node->mNumChildren; i++)
        processNode(node->mChildren[i], scene, meshes);
}

//////////////////////////////////////////////////////////////
// Shader Source
//////////////////////////////////////////////////////////////

const char* vertexShaderSource = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aTex;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoord = aTex;
    gl_Position = projection * view * model * vec4(aPos,1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texture1;

void main()
{
    FragColor = texture(texture1, TexCoord);
}
)";

//////////////////////////////////////////////////////////////
// Main
//////////////////////////////////////////////////////////////

int main()
{
    glfwInit();

    GLFWwindow* window =
        glfwCreateWindow(800, 600, "OpenGL", nullptr, nullptr);

    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);

    //////////////////////////////////////////////////////////
    // Shader compile
    //////////////////////////////////////////////////////////

    auto compile = [](GLenum type, const char* src)
        {
            GLuint s = glCreateShader(type);
            glShaderSource(s, 1, &src, nullptr);
            glCompileShader(s);
            return s;
        };

    GLuint vs = compile(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    //////////////////////////////////////////////////////////
    // Runtime Data
    //////////////////////////////////////////////////////////

    std::vector<Mesh> meshes;
    std::unordered_map<int, GLuint> textures;

    bool ctrlO_pressed = false;

    //////////////////////////////////////////////////////////
    // Loop
    //////////////////////////////////////////////////////////

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(program);

        // Ctrl+O 載模型
        bool ctrl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
        bool oKey = glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS;

        if (ctrl && oKey && !ctrlO_pressed)
        {
            ctrlO_pressed = true;

            std::string path = OpenFileDialog();
            if (!path.empty())
            {
                meshes.clear();
                textures.clear();

                loadFBXModel(path, meshes, textures);
            }
        }

        if (!oKey) ctrlO_pressed = false;

        //////////////////////////////////////////////////////
        // Camera Matrix
        //////////////////////////////////////////////////////

        glm::mat4 model = glm::mat4(1.0f);

        glm::mat4 view =
            glm::lookAt(cameraPos,
                cameraPos + cameraFront,
                cameraUp);

        glm::mat4 projection =
            glm::perspective(glm::radians(45.0f),
                800.0f / 600.0f,
                0.1f,
                100.0f);

        glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &model[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, &projection[0][0]);

        //////////////////////////////////////////////////////
        // Draw Mesh
        //////////////////////////////////////////////////////

        for (auto& m : meshes)
        {
            if (textures.count(m.materialID))
            {
                glBindTexture(GL_TEXTURE_2D, textures[m.materialID]);
            }

            glBindVertexArray(m.vao);
            glDrawArrays(GL_TRIANGLES, 0, m.vertexCount);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}