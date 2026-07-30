# LavaVK 0.9.0-indev

> Currently in development.

A modern, user-friendly C++ wrapper around the Vulkan graphics API.

LavaVK is a lightweight C++20 wrapper around Vulkan that removes boilerplate while preserving Vulkan's architecture.

Unlike game engines or rendering frameworks, LavaVK does not impose an engine design. Every Vulkan object remains familiar, but is wrapped in modern RAII classes with sensible defaults, automatic resource management, and a cleaner API.

LavaVK aims to make Vulkan easier to use while keeping the power and flexibility of the original API. It provides clean C++ abstractions for Vulkan objects without forcing a rendering architecture, allowing users to build their own engines, renderers, tools, and applications.

### Why LavaVK?

LavaVK does not attempt to hide Vulkan.

Instead, it removes boilerplate while keeping Vulkan concepts intact.

If you know Vulkan, you already know LavaVK.

There are no custom rendering abstractions or engine architecture imposed on users.

**LavaVK wrote 1000's of lines of code so that you dont have to**

---

## Features

### Current Features:

- [x] Modern C++20 interface
- [x] Vulkan instance management
- [x] Physical device enumeration and selection
- [x] Logical device creation
- [x] Queue abstraction
- [x] Command buffers
- [x] Swapchain management
- [x] Buffer and image helpers
- [x] Shader management
- [x] Pipeline creation helpers
- [x] Vulkan validation layer support
- [x] Automatic GLSSL → SPIR-V compilation using shaderc
- [x] Supports loading SPIR-V binaries directly
- [x] Compatible with any Window API
- [x] Buffers
- [x] Textures
---
### Planned Features:
- [ ] Compute Pipelines
- [ ] Descriptor indexing
- [ ] Raytracing
- [ ] Dynamic Rendering
- [ ] Mesh Shaders

---

## Reqirements & dependencies
- Vulkan SDK Download: https://vulkan.lunarg.com/
- shaderc (included in repo)
- GLM (included in repo)
- stbi_image (included in repo)

## Installation
```bash
git clone https://github.com/RanveerisdeGOAT/LavaVK.git
```
### Cmake support
```cmake
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

## Documentation
Visit https://ranveerisdegoat.github.io/LavaVK/html/index.html for full documentation

## Getting started
Visit the wiki: https://github.com/RanveerisdeGOAT/LavaVK/wiki/Getting-Started



## Contributing

Contributions are welcome! Please open an issue before submitting large changes.

---

LavaVK 2026
Author: @RanveerisdeGOAT
Co-Authors: Deepseek, Gemini, ChatGPT ;)
Open source: Free to use, modify and improve: https://github.com/RanveerisdeGOAT?tab=repositories
MIT Licence
