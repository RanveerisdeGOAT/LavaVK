cmake_minimum_required(VERSION 4.0)

project(LavaVK VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Vulkan
find_package(Vulkan REQUIRED)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(external/glfw)

# GLM
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glm/glm)

# LavaVK
add_library(LavaVK
        src/Instance.cpp
        src/Device.cpp
        src/Queue.cpp
        src/Fence.cpp
        include/LavaVK/Surface.hpp
        src/Pipeline.cpp
)

target_include_directories(LavaVK
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(LavaVK
        PUBLIC
        Vulkan::Vulkan
        glfw
        glm
)

target_compile_features(LavaVK
        PUBLIC
        cxx_std_20
)

add_library(LavaVK::LavaVK ALIAS LavaVK)