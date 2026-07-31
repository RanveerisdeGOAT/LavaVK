# LavaVK 0.9.1-indev

> Currently in development.

A modern, user-friendly C++ wrapper around the Vulkan graphics API.

LavaVK is a lightweight C++20 wrapper around Vulkan that removes boilerplate while preserving Vulkan's architecture.

Unlike game engines or rendering frameworks, LavaVK does not impose an engine design. Vulkan concepts remain familiar, but are wrapped in modern RAII classes with sensible defaults, automatic resource management, and a cleaner C++ API.

LavaVK aims to make Vulkan easier to use while keeping the power and flexibility of the original API. It provides clean C++ abstractions for Vulkan objects without forcing a rendering architecture, allowing users to build their own engines, renderers, tools, and applications.

## Why LavaVK?

LavaVK does not attempt to hide Vulkan.

Instead, it removes boilerplate while keeping Vulkan concepts intact.

> **If you know Vulkan, you already know LavaVK.**

There are no custom rendering abstractions or engine architecture imposed on users.

**LavaVK handles thousands of lines of Vulkan boilerplate so you don't have to.**

---

## Features

### Current Features

* [x] Modern C++20 interface
* [x] Vulkan instance management
* [x] Physical device enumeration and selection
* [x] Logical device creation
* [x] Queue abstraction
* [x] Command pools and command buffers
* [x] Swapchain management
* [x] Buffer management
* [x] Texture and image management
* [x] Shader management
* [x] Graphics pipeline creation
* [x] Vulkan validation layer support
* [x] Automatic GLSL → SPIR-V compilation using Shaderc
* [x] Direct SPIR-V binary loading
* [x] Window-system-independent core API

### Planned Features

* [ ] Instancing
* [ ] Compute pipelines
* [ ] Descriptor indexing
* [ ] Dynamic rendering

### Maybe in the Far Future ;)

* [ ] Ray tracing
* [ ] Mesh shaders
* [ ] Game-engine functionality?

---

## Requirements & Dependencies

* [Vulkan SDK](https://vulkan.lunarg.com/)
* Shaderc - included in repo
* GLM - included in repo
* STBI image - included in repo

LavaVK itself is designed to be independent of any particular windowing library.

The included examples may use GLFW for window creation and surface management.

---

## Installation

Clone the repository:

```bash
git clone https://github.com/RanveerisdeGOAT/LavaVK.git
```

LavaVK can be included in another CMake project using `add_subdirectory()`:

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

---

## Documentation

Full API documentation is available here:

https://ranveerisdegoat.github.io/LavaVK/html/index.html

## Getting Started

See the [Getting Started wiki](https://github.com/RanveerisdeGOAT/LavaVK/wiki/Getting-Started) for a step-by-step introduction to using LavaVK.

---

## Contributing

Contributions are welcome!

For large changes, please open an issue first to discuss the proposed change.

---

## License

LavaVK is open source and released under the **MIT License**.

You are free to use, modify, and improve LavaVK in accordance with the license.

---

LavaVK 2026
Author: @RanveerisdeGOAT

**Special thanks to everyone who has used, contributed to, or even considered using LavaVK ;).**

*Development of LavaVK has also benefited from AI-assisted development using DeepSeek, Gemini, and ChatGPT. ;)*
