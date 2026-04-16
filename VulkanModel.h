#pragma once
#define GLFW_INCLUDE_VULKAN
#include "VulkanContext.h"
#include "ResourceManager.h"
#include "Vertex.h"

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
    void init(VulkanContext &context, ResourceManager &resourceManager, const std::string &modelPath);

    void cleanup();
    
private:
    VulkanContext *context = nullptr;
    ResourceManager *resourceManager = nullptr;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;    
    std::string modelPath;

    void load(const std::string &modelPath);
};