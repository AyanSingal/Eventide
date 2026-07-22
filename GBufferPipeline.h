#pragma once
#define GLFW_INCLUDE_VULKAN

#include "VulkanContext.h"
#include "ResourceManager.h"
#include "CommandManager.h"
#include "VulkanSwapchain.h"
#include "VulkanModel.h"
#include "Camera.h"
#include "ShaderUtils.h"

struct GBufferUniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
};

class GBufferPipeline {
public:
    void init(VulkanContext& context, ResourceManager& resourceManager
        ,CommandManager& commandManager, VulkanSwapchain& swapchain, VulkanModel& model, Camera& camera, const std::vector<glm::mat4>& modelMatrices);
    void updateUBO();
    void recordCommandBuffer(VkCommandBuffer commandBuffer);
    void cleanup();

    VkImage positionImage;
    VkImageView positionImageView;
    VkImage normalImage;
    VkImageView normalImageView;
    VkImage albedoImage;
    VkImageView albedoImageView;
    VkImageView depthImageView;
    VkSampler gbufferSampler;

    const VkFormat positionFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    const VkFormat normalFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    const VkFormat albedoFormat = VK_FORMAT_R8G8B8A8_UNORM;

private:
    VulkanContext* context = nullptr;
    ResourceManager* resourceManager = nullptr;
    CommandManager* commandManager = nullptr;
    VulkanSwapchain* swapchain = nullptr;
    VulkanModel* model = nullptr;
    Camera* camera = nullptr;

    const std::vector<glm::mat4>* modelMatrices;
    VkDeviceMemory positionImageMemory;
    VkDeviceMemory normalImageMemory;
    VkDeviceMemory albedoImageMemory;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;

    VkDescriptorSetLayout uboSetLayout;
    VkDescriptorSetLayout materialSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet uboDescriptorSet;
    std::vector<VkDescriptorSet> materialDescriptorSets;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    VkBuffer uboBuffer;
    VkDeviceMemory uboMemory;
    void* uboMapped;

    bool firstFrame = true;

    void createGBufferImages();
    void createSampler();
    void createDescriptorSetLayout();
    void createPipeline();
    void createUniformBuffer();
    void createDescriptorPool();
    void createDescriptorSets();
};