#pragma once
#define GLFW_INCLUDE_VULKAN
#include "VulkanContext.h"
#include "CommandManager.h"
#include "ResourceManager.h"
#include "Vertex.h"
#include "VulkanTexture.h"

struct SubMesh{
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    uint32_t indexCount;
    int materialIndex;
};

class VulkanModel
{
public:
    std::vector<SubMesh> subMeshes;
    void init(VulkanContext &context, ResourceManager &resourceManager, CommandManager &commandManager, const std::string &modelPath);

    void cleanup();
    std::vector<VulkanTexture> textures;
    
private:
    VulkanContext *context = nullptr;
    ResourceManager *resourceManager = nullptr;
    CommandManager *commandManager = nullptr;

    void load(const std::string &modelPath);
};