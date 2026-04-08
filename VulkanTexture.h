#pragma once
#define GLFW_INCLUDE_VULKAN

#include <cstring>

#include "VulkanContext.h"
#include "ResourceManager.h"
#include "VulkanTypes.h"
#include "CommandManager.h"

class VulkanTexture{
public:
    VkImageView textureImageView;
    VkSampler textureSampler;

    void init(VulkanContext& context, ResourceManager& resourceManager, CommandManager& commandManager, const std::string &texturePath);
    
    void cleanup();

private:
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    
    uint32_t mipLevels;

    VulkanContext* context = nullptr;
    ResourceManager* resourceManager = nullptr;
    CommandManager* commandManager = nullptr;
    
    void createTextureImage(std::string texturePath);
    void createTextureImageView();
    void createTextureSampler();
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
};