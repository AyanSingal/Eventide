#pragma once
#define GLFW_INCLUDE_VULKAN

#include "VulkanContext.h"
#include "ResourceManager.h"
#include "CommandManager.h"
#include "VulkanSwapchain.h"
#include "GBufferPipeline.h"
#include "Camera.h"
#include "ShaderUtils.h"

struct SSRQueryUBO {
    glm::vec4 rayOrigin;
    glm::vec4 rayDirection;
    glm::mat4 view;
    glm::mat4 proj;
    float maxDistance;
    float stepSize;
    float pad0;
    float pad1;
};

struct SSRQueryResult {
    glm::vec4 hitPosition;
    glm::vec4 hitNormal;
    glm::vec4 hitAlbedo;
    int hit;
    int pad0;
    int pad1;
    int pad2;
};

class SSRQueryPipeline {
public:
    void init(VulkanContext& context, ResourceManager& resourceManager, CommandManager& commandManager,
              VulkanSwapchain& swapchain, GBufferPipeline& gbufferPipeline, Camera& camera);
    void updateQuery();
    void recordCommandBuffer(VkCommandBuffer commandBuffer);
    SSRQueryResult getResult();
    void cleanup();

private:
    VulkanContext* context = nullptr;
    ResourceManager* resourceManager = nullptr;
    CommandManager* commandManager = nullptr;
    VulkanSwapchain* swapchain = nullptr;
    GBufferPipeline* gbufferPipeline = nullptr;
    Camera* camera = nullptr;

    VkDescriptorSetLayout gbufferSetLayout;
    VkDescriptorSetLayout querySetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet gbufferDescriptorSet;
    VkDescriptorSet queryDescriptorSet;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    VkBuffer queryUboBuffer;
    VkDeviceMemory queryUboMemory;
    void* queryUboMapped;

    VkBuffer resultBuffer;
    VkDeviceMemory resultMemory;
    void* resultMapped;

    void createDescriptorSetLayouts();
    void createPipeline();
    void createBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
};