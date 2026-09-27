# Eventide

A Vulkan 1.2 research renderer, written from scratch in C++, for real-time rendering through realistic camera lenses. Instead of a pinhole camera, rays are bent through a real multi-element lens using a precomputed neural lens model. This is a real-time take on [*Precomputed Lens Transport Maps*](https://arxiv.org/abs/2605.04017). A hardware ray tracer renders the same scene as a reference.

<!-- Screenshot: lens view of Sponza with depth of field -->

## Lens camera

Each frame:

1. **G-buffer pass.** The scene is rasterized into position, normal, albedo, and depth images.
2. **Lens pass (compute).** For every pixel, a point on the lens aperture is sampled. The lens model maps the ray through the full lens, a 24mm prescription in [`lens/24mm.json`](lens/24mm.json). The model is a small tanh MLP evaluated directly in the shader. The outgoing ray is then marched against the G-buffer to find what it hits.
3. **Accumulation.** Each frame's aperture sample is averaged into a floating-point image. Over many frames this integrates over the aperture, producing depth of field and the lens's own aberrations. The average restarts when the camera or a lens setting changes.

Controls in the **Lens** panel:

- **Focus.** The sensor distance behind the lens.
- **Aperture.** How much of the lens opening is sampled. Like an f-stop: 0 behaves like a pinhole.
- **Pupil center.** Which part of the aperture is sampled.
- **Accumulated** (in the Lens View window) toggles between the running average and a single sample.

Current limitations:

- The lens pass only sees what the G-buffer contains. Blurred foreground edges can't reveal geometry hidden behind them.
- A single wavelength (550nm) is used, and the model's intensity output is not yet applied.
- The lens view is shown in an ImGui window rather than as the main frame.

## Features

### Core
- **Vulkan 1.2** with `VK_KHR_dynamic_rendering` (no render passes)
- **Timeline semaphores** for frame-in-flight synchronization
- **Dedicated transfer queue** for asynchronous GPU uploads
- **glTF 2.0 model loading** with per-material base color textures
- **FPS fly camera** with click-drag rotation and WASD movement
- **ImGui UI** for lens controls, frame timing, and a lens-vs-ray-traced hit comparison

### Lens camera
- **G-buffer pass** (world position, normal, albedo, depth)
- **Compute lens pass** with the neural lens model evaluated per pixel; weights are loaded into a storage buffer
- **Screen-space ray marching** with binary-search refinement
- **Temporal accumulation** of aperture samples in an RGBA32F image, using a low-discrepancy (R2) sample sequence

### Ray tracing (reference view)
- **Acceleration structures** built from glTF geometry (one BLAS per sub-mesh, single TLAS)
- **Ray tracing pipeline** with ray generation, closest-hit, and miss shaders
- **Lambertian shading with shadow rays**
- **Shader Binding Table** with properly aligned shader group regions
- **Runtime descriptor arrays** indexing per-sub-mesh vertex/index buffers and per-material textures

### Rasterization path (present, not currently displayed)
- **MSAA** with automatic resolve
- **Mipmapped textures** generated via blit chain
- **Per-material descriptor sets** and **push constants** for model matrices

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
|-- Camera              : FPS fly camera, view/projection matrices
|-- GBufferPipeline     : G-buffer rasterization pass
|-- LensModel           : Loads the lens network weights and normalization constants
|-- SSRQueryPipeline    : Lens compute pass: aperture sampling, network evaluation, screen-space march, accumulation
|-- RayTracingAS        : BLAS/TLAS acceleration structure construction
|-- RayTracingPipeline  : RT pipeline, shader binding table, storage image, RT descriptors
|-- Renderer            : Graphics pipeline, descriptors, sync, UI, draw loop orchestration
```

Shared headers: `VulkanTypes.h` (queue/swapchain structs, validation and extension config), `Vertex.h` (vertex layout, UBO), `ShaderUtils.h` (shader file reading and module creation).

Data: `lens/` holds the lens prescription and the network weights with their spec. `shaders/` holds the GLSL sources, which CMake compiles to SPIR-V.

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

### Test scene

The default scene is [Sponza](https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/Sponza/glTF). It is CRYENGINE-licensed, so it is not included in this repository. Place the contents of its `glTF/` folder in `models/Sponza/` before running.

To use the included FlightHelmet model instead, point `MODEL_PATH` in `main.cpp` at `models/FlightHelmet/FlightHelmet.gltf`. You may also need to adjust the model transform in `main.cpp` and the starting camera in `Camera.cpp`.

### Build

```bash
cmake -S . -B build
cmake --build build
```

The executable, compiled shaders, models, and textures are output to `build/`.

## Roadmap

**Done**
- Modular Vulkan foundation, glTF loading, material system, ImGui UI
- Hardware ray tracing with shading and shadow rays
- Real-time neural lens camera with depth of field, focus, and aperture control

**Next**
- Apply the lens model's intensity (Fresnel throughput) output
- Spectral sampling for chromatic aberration
- Present the lens view as the main frame
- Performance work toward a real-time frame budget
- Validation against ray-traced lens references

**Longer term**
- Neural importance sampling for light transport, beginning in non-Euclidean settings

## References

- Yang Chen, Xiaochun Tong, Afet Abzar, Leo Hanxu, Matthew Avolio, Toshiya Hachisuka. *Precomputed Lens Transport Maps.* arXiv:2605.04017, 2026.
- Lens prescription and data-collection reference: [noviorlu/cs888-public](https://github.com/noviorlu/cs888-public)

## License

This is a personal research project. No license specified.
