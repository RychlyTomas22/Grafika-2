cmake_minimum_required(VERSION 3.30)

project(icp VERSION 2025.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

find_package(glm           CONFIG REQUIRED)
find_package(GLEW                 REQUIRED)
find_package(glfw3         CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui         CONFIG REQUIRED)

find_package(OpenCV CONFIG REQUIRED)

set(PROJECT_SOURCES
        glfw_callbacks.cpp
        gl_utils.cpp
        gl_err_callback.cpp
        app.cpp
        main.cpp
        ShaderProgram.cpp
        OBJloader.cpp
        Texture.cpp
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE
        GLFW_INCLUDE_NONE
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        GLEW::GLEW
        glfw
        glm::glm
        nlohmann_json::nlohmann_json
        imgui::imgui
        ${OpenCV_LIBS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${OpenCV_INCLUDE_DIRS}
)