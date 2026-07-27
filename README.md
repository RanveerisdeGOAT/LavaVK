# LavaVK

A modern, user-friendly C++ wrapper around the Vulkan graphics API.

LavaVK aims to make Vulkan easier to use while keeping the power and flexibility of the original API. It provides clean C++ abstractions for Vulkan objects without forcing a rendering architecture, allowing users to build their own engines, renderers, tools, and applications.

---

## Features

Currently in development.

Planned features:

- Modern C++20 interface
- Vulkan instance management
- Physical device enumeration and selection
- Logical device creation
- Queue abstraction
- Command buffers
- Swapchain management
- Buffer and image helpers
- Shader management
- Pipeline creation helpers
- Vulkan validation layer support

---

## Reqirements & dependencies
- Vulkan SDK Download: https://vulkan.lunarg.com/
- shaderc (included in repo)
- GLM (included in repo)

## Installation
```bash
git clone https://github.com/RanveerisdeGOAT/LavaVK.git
```
###Cmake support
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
Visit https://ranveerisdegoat.github.io/LavaVK/html/index.html for documentation

## Contributing

Contributions are welcome! Please open an issue before submitting large changes.

---

LavaVK 2026
Author: @RanveerisdeGOAT
Co-Authors: Deepseek, Gemini, ChatGPT ;)
Open source: Free to use, modify and improve: https://github.com/RanveerisdeGOAT?tab=repositories
MIT Licence
