cmake_minimum_required(VERSION 3.10)
project(MyOpenGLApp)

# 使用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/glm")

# 2. 設定 GLFW 與 GLAD
set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw")
include_directories(${GLFW_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include)

# 3. 加入 tinyobjloader 與其他自定義標頭檔
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

# 4. 加入 Assimp
set(ASSIMP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/Dependencies/assimp")
include_directories(${ASSIMP_DIR}/include)

# 5. 加入 vcpkg (如果還有缺少的庫)
include_directories("C:/engine/OpenGLProject/vcpkg-master/installed/x64-windows/include")

# 函式庫與執行檔設定

# 建立 GLAD 庫
add_library(glad STATIC external/glad/src/glad.c)

# 連結庫路徑
link_directories("${ASSIMP_DIR}/lib")
link_directories("${ASSIMP_DIR}/lib/x64")

# 收集 src 下所有 cpp
file(GLOB SRC_FILES src/*.cpp)

# 建立執行檔
add_executable(MyOpenGLApp
    src/main.cpp
    src/Camera/Camera.cpp
    src/Camera/InputManager.cpp
    src/engin/Mesh.cpp
    src/ModelLoader/ModelLoader.cpp
    src/platform/FileDialog.cpp
    src/Renderer/Renderer.cpp
    src/Shader/Shader.cpp
    src/Texture/Texture.cpp
    "${CMAKE_CURRENT_SOURCE_DIR}/include/tiny_obj_loader.cc"
)
target_include_directories(MyOpenGLApp PRIVATE 
    src/Camera
    src/engin
    src/ModelLoader
    src/Renderer
    src/Shader
    src/platform
    src/Texture
)
# 連結必要的函式庫
target_link_libraries(MyOpenGLApp 
    glad 
    glfw3 
    opengl32 
    assimp-vc143-mt
)

# MSVC 執行階段設定
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")