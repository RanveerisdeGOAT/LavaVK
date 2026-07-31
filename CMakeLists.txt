cmake_minimum_required(VERSION 3.20)

project(LavaVK
        VERSION 0.11.0
        LANGUAGES CXX
)

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

add_library(LavaVK::LavaVK ALIAS LavaVK)


find_program(GLSLC_EXECUTABLE glslc REQUIRED)


target_include_directories(LavaVK
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>

        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/external
)


target_link_libraries(LavaVK
        PUBLIC
        Vulkan::Vulkan
)


target_compile_features(LavaVK
        PUBLIC
        cxx_std_20
)


#
# Installation
#

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)


install(
        TARGETS LavaVK
        EXPORT LavaVKTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)


install(
        DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)


#
# Export targets
#

install(
        EXPORT LavaVKTargets
        FILE LavaVKTargets.cmake
        NAMESPACE LavaVK::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LavaVK
)


#
# Package config
#

configure_package_config_file(
        cmake/LavaVKConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/LavaVKConfig.cmake
        INSTALL_DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/LavaVK
)


write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/LavaVKConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)


install(
        FILES
        ${CMAKE_CURRENT_BINARY_DIR}/LavaVKConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/LavaVKConfigVersion.cmake
        DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/LavaVK
)