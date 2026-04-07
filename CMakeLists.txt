cmake_minimum_required(VERSION 3.20)
project(VulkanApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ──────────────────────────────────────────────
# Put the exe directly in build/ instead of build/Debug/.
# This way relative paths like "shaders/vert.spv" and
# "models/viking_room.obj" work without fuss.
# ──────────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR})

# ──────────────────────────────────────────────
# 1. Find Vulkan SDK (uses the VULKAN_SDK env variable you already have)
# ──────────────────────────────────────────────
find_package(Vulkan REQUIRED)

# ──────────────────────────────────────────────
# 2. Third-party library paths
#    All dependencies live in external/ inside the repo.
#    Expected layout:
#      external/
#        glfw-3.4.bin.WIN64/    (include/ and lib-vc2022/)
#        glm/
#        stb-master/
#        tinyobjloader/
# ──────────────────────────────────────────────
set(LIB_DIR "${CMAKE_SOURCE_DIR}/external")

set(GLFW_INCLUDE_DIR "${LIB_DIR}/glfw-3.4.bin.WIN64/include")
set(GLFW_LIB         "${LIB_DIR}/glfw-3.4.bin.WIN64/lib-vc2022/glfw3.lib")
set(GLM_INCLUDE_DIR  "${LIB_DIR}/glm")
set(STB_INCLUDE_DIR  "${LIB_DIR}/stb-master")
set(TINYOBJ_INCLUDE_DIR "${LIB_DIR}/tinyobjloader")

# ──────────────────────────────────────────────
# 3. Build the executable
# ──────────────────────────────────────────────
add_executable(${PROJECT_NAME} main.cpp VulkanContext.cpp CommandManager.cpp ResourceManager.cpp VulkanSwapchain.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    ${STB_INCLUDE_DIR}
    ${TINYOBJ_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
    ${GLFW_LIB}
)

# ──────────────────────────────────────────────
# 4. Shader compilation
#    Finds glslangValidator from the Vulkan SDK and compiles each
#    .vert / .frag file into a .spv file in the build's shaders/ dir.
#    Re-runs automatically when you edit a shader.
# ──────────────────────────────────────────────
find_program(GLSLANG_VALIDATOR glslangValidator
    HINTS "$ENV{VULKAN_SDK}/Bin"
)

if(NOT GLSLANG_VALIDATOR)
    message(FATAL_ERROR "glslangValidator not found — is your VULKAN_SDK env variable set?")
endif()

set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders")

file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

# List your shader source files here. Add new ones as you create them.
set(SHADER_SOURCES
    ${SHADER_SOURCE_DIR}/shader.vert
    ${SHADER_SOURCE_DIR}/shader.frag
)

foreach(SHADER_SOURCE ${SHADER_SOURCES})
    # Extract filename: "shader.vert" -> "vert.spv"
    get_filename_component(SHADER_EXT ${SHADER_SOURCE} LAST_EXT)   # ".vert"
    string(SUBSTRING ${SHADER_EXT} 1 -1 SHADER_STAGE)              # "vert"
    set(SHADER_OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_STAGE}.spv")

    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER_SOURCE} -o ${SHADER_OUTPUT}
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling ${SHADER_STAGE} shader"
        VERBATIM
    )

    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
endforeach()

# This custom target makes the shader compilation part of the build.
# "ALL" means it runs every time you build the project.
add_custom_target(compile_shaders ALL DEPENDS ${SHADER_OUTPUTS})
add_dependencies(${PROJECT_NAME} compile_shaders)

# ──────────────────────────────────────────────
# 5. Copy runtime assets (models, textures) to build directory
#    Your code loads these via relative paths like "models/viking_room.obj",
#    so they need to be next to the executable.
# ──────────────────────────────────────────────
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/models"
        "${CMAKE_BINARY_DIR}/models"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/textures"
        "${CMAKE_BINARY_DIR}/textures"
    COMMENT "Copying models and textures to build directory"
)