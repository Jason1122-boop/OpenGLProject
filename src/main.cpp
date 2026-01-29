#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>          
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <windows.h>
#include <commdlg.h>
#include "tiny_obj_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <assimp/postprocess.h>
#include <assimp/BaseImporter.h>
#include <assimp/material.h>
#include <assimp/scene.h>
#include "../build/Dependencies/assimp/samples/SimpleTexturedDirectx11/SimpleTexturedDirectx11/ModelLoader.h"
#include <direct.h>
#include "../build/Dependencies/assimp/samples/SimpleAssimpViewX/ModelLoaderHelperClasses.h"



//存3D點
struct Vec3 { float x, y, z;};

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

    // 2. 處理材質 (貼圖)
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* mat = scene->mMaterials[i];
        aiString str;
        if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &str) == AI_SUCCESS) {
            std::string filename = std::string(str.C_Str());

            // 關鍵：去掉路徑，只拿檔名 (例如 C:\Users\Desktop\Side_View.jpg -> Side_View.jpg)
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

//上帝視角
void processInput(GLFWwindow* window, glm::vec3& cameraPos, glm::vec3& cameraFront, glm::vec3& cameraUp)
{
    float cameraSpeed = 0.05f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

//轉動視角
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 400.0f;
float lastY = 300.0f;
bool firstMouse = true;

glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraup = glm::vec3(0.0f, 1.0f, 0.0f);
//mouse contral
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    static float sensitivity = 0.1f;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 
    lastX = xpos;
    lastY = ypos;

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

struct Mesh {
    GLuint vao;
    GLuint vbo;
    int vertexCount;
    int materialID;
};

// 使用 win32 open file dialog（回傳空字串 = 取消）
std::string OpenFileDialog(const char* filter = "OBJ Files/0*.obj/0All Files/0*.*/0") {
    OPENFILENAMEA ofn;
    CHAR szFile[MAX_PATH] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL; // 可改成你的 HWND（從 glfw 可取得）
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        return std::string(szFile);
    }
    else {
        return std::string(); // empty = cancelled / failed
    }
}

bool loadTexturedModel(const std::string& objPath,
    std::vector<Mesh>& meshes,
    std::unordered_map<int, GLuint>& materialTextures) {

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // 自動解析 base path
    std::string basePath = objPath.substr(0, objPath.find_last_of("/\\") + 1);

    // 建立每個 shape 的 VAO/VBO
    for (const auto& shape : shapes) {
        std::vector<float> vertexData;
        for (const auto& index : shape.mesh.indices) {
            float vx = attrib.vertices[3 * index.vertex_index + 0];
            float vy = attrib.vertices[3 * index.vertex_index + 1];
            float vz = attrib.vertices[3 * index.vertex_index + 2];
            vertexData.push_back(vx);
            vertexData.push_back(vy);
            vertexData.push_back(vz);

            if (!attrib.texcoords.empty() && index.texcoord_index >= 0) {
                float tx = attrib.texcoords[2 * index.texcoord_index + 0];
                float ty = attrib.texcoords[2 * index.texcoord_index + 1];
                vertexData.push_back(tx);
                vertexData.push_back(ty);
            } else {
                vertexData.push_back(0.0f);
                vertexData.push_back(0.0f);
            }
        }

        Mesh mesh;
        glGenVertexArrays(1, &mesh.vao);
        glGenBuffers(1, &mesh.vbo);
        glBindVertexArray(mesh.vao);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        mesh.vertexCount = vertexData.size() / 5;
        mesh.materialID = shape.mesh.material_ids.empty() ? -1 : shape.mesh.material_ids[0];
        meshes.push_back(mesh);
    }


    return true;
}

bool ctrlO_pressed = false;

void setWorkingDirectoryToExePath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    std::string path = std::string(buffer).substr(0, pos);
    _chdir(path.c_str()); // 將當前工作目錄更改為 exe 所在路徑
}

int main() 
{

    setWorkingDirectoryToExePath();
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    
    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    // 初始化 GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    glEnable(GL_DEPTH_TEST);
    std::vector<Mesh> meshes;
    std::unordered_map<int, GLuint> materialTextures;

    //if (!loadFBXModel("./assets/your_model.fbx", meshes, materialTextures)) {
    //    std::cerr << "FBX 模型載入失敗！" << std::endl;
    //}



    //頂點著色器
    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        TexCoord = aTexCoord;
    }
    )";

    const char* fragmentShaderSource = R"(
    #version 330 core
    in vec2 TexCoord;
    out vec4 FragColor;

    uniform sampler2D texture1;

    void main() {
        vec4 texColor = texture(texture1, TexCoord);
    
        // 檢查採樣到的顏色。如果完全是黑的(0,0,0)，代表可能沒讀到貼圖或UV錯了
        // 我們讓它顯示深灰色，這樣你就知道模型其實在那裡，只是沒貼圖
        if (texColor.rgb == vec3(0.0, 0.0, 0.0)) {
            FragColor = vec4(0.3, 0.3, 0.3, 1.0); 
        } else {
            // 如果有讀到顏色，稍微增加亮度(1.2倍)讓你比較好觀察細節
            FragColor = texColor * 1.2; 
        }
    }
    )";
    
    // 編譯 Vertex Shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    //檢查是否成功
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // 編譯 Fragment Shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // 建立 Shader Program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // 檢查是否成功
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);    // 刪掉不需要的 shader

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);    //背景顏色

    

    while (!glfwWindowShouldClose(window)) {
        processInput(window, cameraPos, cameraFront, cameraup);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);   // 使用shader program
        
        // 偵測按鍵狀態
        int ctrl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
        int oKey = glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS;

        if (ctrl && oKey && !ctrlO_pressed)
        {
            ctrlO_pressed = true;
            std::string path = OpenFileDialog("Model Files\0*.fbx;*.obj\0All Files\0*.*\0");
            if (!path.empty())
            {
                meshes.clear();
                materialTextures.clear();

                // 統一交給 Assimp 處理，它會自動偵測是 FBX
                loadFBXModel(path, meshes, materialTextures);
            }
        }
        if (!oKey) ctrlO_pressed = false;

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraup);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

        int modelLoc = glGetUniformLocation(shaderProgram, "model");   // 傳給 shader
        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        int projLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        for (auto& mesh : meshes) {

            if (mesh.materialID >= 0 && materialTextures.count(mesh.materialID)) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, materialTextures[mesh.materialID]);
                glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
            }

            glBindVertexArray(mesh.vao);
            glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
        }

        glBindVertexArray(0);          // 解除綁定 VAO (可選)
        glfwSwapBuffers(window);       //顯示畫面
        glfwPollEvents();              //處理事件
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    system("pause");
    return 0;
}