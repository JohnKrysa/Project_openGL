cmake_minimum_required(VERSION 3.10)
project(OpenGL_Game)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)

find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(MyGame src/main.cpp)

target_link_libraries(MyGame 
    OpenGL::GL 
    GLEW::GLEW 
    glfw
)