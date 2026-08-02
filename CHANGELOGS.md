# Changelog

All notable changes to LavaVK are documented here.

## [0.12.0-release] — 2026-08-02

> **Development status:** Released

### Added
* Added dynamic rendering

---

## [0.14.0-indev] — 2026-08-02

> **Development status:** In development

### Added
* Added descriptor indexing.
* Added bindless textures.

---

## [0.13.0-indev] — 2026-08-01

> **Development status:** In development

### Added
* Finally added compute shaders.

---

## [0.12.0-indev] — 2026-07-31

> **Development status:** In development

### Added
* Added indirect command calls `drawIndirect()` and `drawIndexedIndirect()`

---

## [0.11.0-indev] — 2026-07-31

> **Development status:** In development

### Added
* Added `void copyToBuffer(Buffer &dstBuffer) const;` usefull for stage buffers.

---

## [0.10.1-indev] — 2026-07-31

> **Development status:** In development

### Fixed
* Fixed binding problem in `VertexLayout`.
* Removed Release vs Debug error handler in `Error.hpp`.

---

## [0.10.0-indev] — 2026-07-31

> **Development status:** In development

### Added
* Finally added proper cmake support
#### Installation

Option 1: Add LavaVK as a CMake subdirectory

Clone the repository:

```bash
git clone https://github.com/RanveerisdeGOAT/LavaVK.git
```

Add LavaVK to your project:

```cmake
cmake_minimum_required(VERSION 3.20)

project(Example)

add_subdirectory(LavaVK)

add_executable(Example
    main.cpp
)

target_link_libraries(Example
    PRIVATE
        LavaVK::LavaVK
)
```

Your project structure should look like:

```
Example/
├── CMakeLists.txt
├── main.cpp
└── LavaVK/
    ├── CMakeLists.txt
    ├── include/
    └── src/
```

---

Option 2: Install LavaVK as a CMake package

Clone and build LavaVK:

```bash
git clone https://github.com/RanveerisdeGOAT/LavaVK.git
cd LavaVK

cmake -B build -DCMAKE_INSTALL_PREFIX=/path/to/install
cmake --build build
cmake --install build
```

Then use LavaVK from another CMake project:

```cmake
cmake_minimum_required(VERSION 3.20)

project(Example)

find_package(LavaVK REQUIRED)

add_executable(Example
    main.cpp
)

target_link_libraries(Example
    PRIVATE
        LavaVK::LavaVK
)
```

If CMake cannot find LavaVK, specify the installation path:

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/install
```

---

## [0.9.2-indev] — 2026-07-31

> **Development status:** In development

### Fixed
* Removed `shaderc` dependency, reducing repo size.

---

## [0.9.1-indev] — 2026-07-31

> **Development status:** In development

### Fixed
* Fixed `LavaVK::Textures` move assignment operator.

## [0.9.0-indev] — 2026-07-30

> **Development status:** In development

### Added

* RAII-based Vulkan resource management.
* `LavaVK::Instance` for Vulkan instance creation.
* `LavaVK::GPUHardware` for physical device selection.
* `LavaVK::Device` for logical device management.
* Support for graphics and presentation queues.
* `LavaVK::Surface` abstraction.
* `LavaVK::SwapChain` abstraction.
* `LavaVK::RenderPass` abstraction.
* `LavaVK::GraphicsPipeline` abstraction.
* `LavaVK::Shader` with automatic GLSL-to-SPIR-V compilation.
* Direct SPIR-V shader loading.
* Descriptor set and descriptor set layout abstractions.
* Command pool and command buffer abstractions.
* Buffer abstractions.
* Texture abstractions.
* Synchronization primitives.
* GLFW integration helpers.
* Hello Triangle example.

### Planned

* Instancing.
* Compute pipelines.
* Descriptor indexing.
* Dynamic rendering.

### Future

* Ray tracing.
* Mesh shaders.
* Potential higher-level engine functionality.
