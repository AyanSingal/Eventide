# Eventide

A Vulkan 1.2 renderer built from scratch in C++. Renders a textured glTF model through two independent paths: a traditional rasterizer and a hardware-accelerated ray tracer built on Vulkan's ray tracing extensions.

## Features

### Core
- **Vulkan 1.2** with `VK_KHR_dynamic_rendering` (no render passes)
- **Timeline semaphores** for frame-in-flight synchronization (replacing traditional fences)
- **Dedicated transfer queue** for asynchronous GPU uploads
- **Modular architecture** with clear separation of concerns across well-defined modules

### Rasterization path
- **MSAA** with automatic resolve
- **Mipmapped textures** generated via blit chain
- **glTF 2.0 model loading** with per-material base color textures
- **Per-material descriptor sets** (set 0 = per-frame UBO, set 1 = per-material texture)
- **Push constants** for per-object model matrices
- **FPS fly camera** with click-drag rotation and WASD movement
- **ImGui debug UI** with camera position and frame timing

### Ray tracing path (hardware accelerated)
- **Acceleration structures** built from glTF geometry (one BLAS per sub-mesh, single TLAS)
- **Ray tracing pipeline** with ray generation, closest-hit, and miss shaders
- **Shader Binding Table** with properly aligned shader group regions
- **Storage image output** copied to the swapchain for presentation
- **Per-material texture sampling** in the closest-hit shader via barycentric UV interpolation
- **Runtime descriptor arrays** indexing per-sub-mesh vertex/index buffers and per-material textures


<img width="483" height="510" alt="image" src="https://github.com/user-attachments/assets/3c64efe9-e636-41fd-87d0-d8b1c90b2dcd" />

## Architecture

```
main.cpp (application entry, draw loop, scene-specific logic)
|
|-- VulkanContext       : Instance, device, queues, debug messenger, RT function pointers
|-- CommandManager      : Command pools, command buffers, one-shot submissions
|-- ResourceManager     : Buffer/image creation, memory allocation, data transfer
|-- VulkanSwapchain     : Swapchain lifecycle, image views, MSAA/depth resources
|-- VulkanTexture       : Texture creation, management, mipmaps
|-- VulkanModel         : glTF model loading, per-sub-mesh buffers, per-material textures
|-- RayTracingAS        : BLAS/TLAS acceleration structure construction
|-- RayTracingPipeline  : RT pipeline, shader binding table, storage image, RT descriptors
|-- Renderer            : Graphics pipeline, descriptors, sync, draw loop orchestration
|-- Camera              : FPS fly camera, view/projection matrices
```

Shared headers: `VulkanTypes.h` (queue/swapchain structs, validation and extension config), `Vertex.h` (vertex layout, UBO), `ShaderUtils.h` (shader file reading and module creation).

## Building

### Prerequisites

- [Vulkan SDK](https://vulkan.lunarg.com/) (1.2 or newer, with ray tracing support)
- [CMake](https://cmake.org/) 3.20+
- MSVC toolchain (Visual Studio 2022 or Build Tools)
- A GPU with hardware ray tracing support (`VK_KHR_ray_tracing_pipeline`, `VK_KHR_acceleration_structure`)
- Dependencies (place in `external/`):
  - [GLFW 3.4](https://www.glfw.org/) (prebuilt WIN64 binaries)
  - [GLM](https://github.com/g-truc/glm)
  - [stb](https://github.com/nothings/stb)
  - [tinygltf](https://github.com/syoyo/tinygltf)
  - [Dear ImGui](https://github.com/ocornut/imgui) (docking branch)

### Build

```bash
cmake -S . -B build
cmake --build build
```

The executable, compiled shaders, models, and textures are output to `build/`.

## Roadmap

**Phase 1: Foundation refactor (complete)**
Breaking the monolith into clean modules with clear separation of concerns.

**Phase 2: Renderer features (complete)**
Scene abstraction, glTF loading, material system, ImGui debug UI, push constants, multiple descriptor sets.

**Phase 3: Toward path tracing (in progress)**
Hardware ray tracing infrastructure is complete: acceleration structures, ray tracing pipeline, shader binding table, and textured output via ray casting. Remaining work moves from ray casting to true light transport:
- Surface normals and basic direct lighting
- Shadow rays (first secondary rays)
- Monte Carlo path tracing with multiple bounces and sample accumulation
- Research direction: neural importance sampling for light transport, beginning in non-Euclidean settings

## License

This is a personal learning project. No license specified.
