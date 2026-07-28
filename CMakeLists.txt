cmake_minimum_required(VERSION 3.20) # Avoid CMP0100/CMP0180 issues with newer CMake versions

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

# SHADERC (Disable all install targets for sub-dependencies)
set(SHADERC_SKIP_INSTALL ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_COPYRIGHT_CHECK ON CACHE BOOL "" FORCE)

set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_INSTALL OFF CACHE BOOL "" FORCE) # Key fix for glslang export error
set(SPIRV_SKIP_INSTALL ON CACHE BOOL "" FORCE)
set(SPIRV_HEADERS_SKIP_INSTALL ON CACHE BOOL "" FORCE)
set(ENABLE_CTEST OFF CACHE BOOL "" FORCE)

# Pass EXCLUDE_FROM_ALL so CMake ignores install targets inside shaderc
add_subdirectory(external/shaderc EXCLUDE_FROM_ALL)

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
        shaderc
)

target_compile_features(LavaVK
        PUBLIC
        cxx_std_20
)

add_library(LavaVK::LavaVK ALIAS LavaVK)