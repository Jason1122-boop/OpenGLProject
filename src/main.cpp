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




//存3D點
struct Vec3 { float x, y, z;};

//OBJ讀取函式只讀頂點
bool loadOBJ(const char* path, std::vector<float>& outVertices) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open OBJ file: " << path << std::endl;
        return false;
    }

    std::vector<Vec3> temp_vertices;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string tag; ss >> tag;
        if (tag == "v") {
            Vec3 v; ss >> v.x >> v.y >> v.z;
            temp_vertices.push_back(v);
        } else if (tag == "f"){
            for (int i = 0; i < 3; ++i) {
                std::string tok; ss >> tok;
                if (tok.empty()) break;
                size_t slash = tok.find('/');
                int idx = 0;
                if (slash == std::string::npos) {
                    idx = std::stoi(tok);
                }
                else {
                    idx = std::stoi(tok.substr(0, slash));
                }
                const Vec3& p = temp_vertices[idx - 1];
                outVertices.push_back(p.x);
                outVertices.push_back(p.y);
                outVertices.push_back(p.z);
            }
        }
    }
    return !outVertices.empty();
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

// 使用 Win32 Open File dialog（回傳空字串表示取消）
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

    // 載入 .obj + .mtl
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
        objPath.c_str(), basePath.c_str(), true, true);
    if (!ret) {
        std::cerr << "Failed to load OBJ: " << err << std::endl;
        std::cerr << "Warning: " << warn << std::endl;
        return false;
    }

    // 載入所有材質的貼圖******
    for (size_t i = 0; i < materials.size(); ++i) {
        std::string texname = materials[i].diffuse_texname;
        std::string texturePath;

        if (texname.find(":") != std::string::npos || texname.find("/") == 0 || texname.find("\\") == 0) {
            texturePath = texname;
        } else {
            texturePath = basePath + texname;
        }

        int width, height, channels;
        unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &channels, 0);
        if (!data) {
            std::cerr << "Failed to load texture: " << texturePath << std::endl;
            continue;
        }

        GLuint texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);
        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);

        materialTextures[i] = texID;
    }

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

int main() {
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

    if (!loadTexturedModel("C:/Users/USER/Downloads/temu2000.mtl.obj", meshes, materialTextures)) {
        std::cerr << "模型載入失敗！" << std::endl;
    }

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
    FragColor = texture(texture1, TexCoord);
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
        glUseProgram(shaderProgram);   // 使用我們的 shader program
        
        int ctrlState = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
        int oState = glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS;

        if (ctrlState && oState && !ctrlO_pressed) {
            ctrlO_pressed = true;

            // 彈出檔案選擇對話框（同步）
            std::string path = OpenFileDialog("OBJ Files/0*.obj/0All Files/0*.*/0");
            if (!path.empty()) {
                std::cout << "Trying to load: " << path << std::endl;

                // 把新模型 append 到 scenes（loadTexturedModel 已經會 push_back mesh）
                if (!loadTexturedModel(path, meshes, materialTextures)) {
                    std::cerr << "Failed to load model: " << path << std::endl;
                }
                else {
                    std::cout << "Model loaded and appended to scene: " << path << std::endl;
                }
            }
        }
        // release debounce when O released
        if (!oState) ctrlO_pressed = false;

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