# Changelog

All notable changes to LavaVK are documented here.

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

### Notes

`0.9.0-indev` is a development release. The public API may change before the `1.0.0` release.

The `1.0.0` release will mark the first stable LavaVK API.
