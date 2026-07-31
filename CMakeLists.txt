cmake_minimum_required(VERSION 3.20) # Avoid CMP0100/CMP0180 issues with newer CMake versions

project(LavaVK VERSION 0.9.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Vulkan
find_package(Vulkan REQUIRED)

# GLM
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glm/glm)

# LavaVK
add_library(LavaVK
        src/Instance.cpp
        src/Device.cpp
        src/Queue.cpp
        src/Sync.cpp
        include/LavaVK/Surface.hpp
        src/Pipeline.cpp
        src/SwapChain.cpp
        src/Buffer.cpp
        src/Command.cpp
        src/Shader.cpp
        src/Descriptor.cpp
        include/LavaVK/Core.hpp
        src/Texture.cpp
)

find_program(GLSLC_EXECUTABLE glslc REQUIRED)

target_include_directories(LavaVK
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(LavaVK
        PUBLIC
        Vulkan::Vulkan
        glm
        shaderc
)

target_compile_features(LavaVK
        PUBLIC
        cxx_std_20
)

add_library(LavaVK::LavaVK ALIAS LavaVK)