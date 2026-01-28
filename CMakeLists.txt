cmake_minimum_required(VERSION 3.10)
project(MyOpenGLApp)

# 使用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 設定 GLFW 路徑
set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw")
include_directories(${GLFW_DIR}/include)
link_directories("${ASSIMP_DIR}/lib/x64")

# 加入 glad（使用本地原始碼）
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include)
add_library(glad STATIC external/glad/src/glad.c)

# 加入 tinyobjloader（單一檔案）
include_directories(${CMAKE_SOURCE_DIR}/include)

# 加入其他 include 路徑（例如 vcpkg 安裝的其他庫）
include_directories("C:/engine/OpenGLProject/vcpkg-master/installed/x64-windows/include")

# 收集所有 .cpp 檔案
file(GLOB SRC_FILES
    src/*.cpp
)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

set(ASSIMP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/Dependencies/assimp")
include_directories(${ASSIMP_DIR}/include)
link_directories(${ASSIMP_DIR}/lib)

# 建立執行檔
add_executable(MyOpenGLApp
    src/main.cpp
    C:/engine/OpenGLProject/include/tiny_obj_loader.cc
    ${SRC_FILES}
)

# 連結必要的函式庫
target_link_libraries(MyOpenGLApp 
    glad 
    glfw3 
    opengl32 
    assimp-vc143-mt  # 確保這檔名跟你 lib 資料夾下的一樣
)