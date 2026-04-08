# Eventide

A Vulkan 1.2 renderer built from scratch in C++. Currently renders a textured, rotating 3D model with MSAA, dynamic rendering, and timeline semaphore synchronization.

## Features

- **Vulkan 1.2** with `VK_KHR_dynamic_rendering` (no render passes)
- **Timeline semaphores** for frame-in-flight synchronization (replacing traditional fences)
- **Dedicated transfer queue** for async GPU uploads
- **MSAA** with automatic resolve
- **Mipmapped textures** generated via blit chain
- **OBJ model loading** with vertex deduplication
- **Modular architecture** — clean separation of concerns across well-defined modules

## Architecture

```
main.cpp (application entry, draw loop, scene-specific logic)
├── VulkanContext       — Instance, device, queues, debug messenger
├── CommandManager      — Command pools, command buffers, one-shot submissions
├── ResourceManager     — Buffer/image creation, memory allocation, data transfer
└── VulkanSwapchain     — Swapchain lifecycle, image views, MSAA/depth resources
└── VulkanTexture       — Texture creation, management, mipmaps
```

Shared headers: `VulkanTypes.h` (queue/swapchain structs, validation config), `Vertex.h` (vertex layout, UBO)

Remaining modules (in progress): Model, Pipeline, Descriptors, Renderer

## Building

### Prerequisites

- [Vulkan SDK](https://vulkan.lunarg.com/) (1.2+)
- [CMake](https://cmake.org/) 3.20+
- MSVC toolchain (Visual Studio 2022 or Build Tools)
- Dependencies (place in `external/`):
  - [GLFW 3.4](https://www.glfw.org/) (prebuilt WIN64 binaries)
  - [GLM](https://github.com/g-truc/glm)
  - [stb](https://github.com/nothings/stb)
  - [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)

### Build

```bash
cmake -S . -B build
cmake --build build
```

The executable, compiled shaders, models, and textures are output to `build/`.

## Roadmap

**Phase 1 — Foundation refactor** (current)
Breaking the monolith into clean modules with clear separation of concerns.

**Phase 2 — Renderer features**
Scene abstraction, glTF loading, material system, ImGui debug UI, push constants, multiple descriptor sets.

**Phase 3 — Path tracing**
Vulkan ray tracing extensions or compute-based path tracing, targeting research in neural importance sampling and light transport.

## License

This is a personal learning project. No license specified.
