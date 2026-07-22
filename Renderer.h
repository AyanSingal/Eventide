#pragma once
#define GLFW_INCLUDE_VULKAN

#include "VulkanContext.h"
#include "ResourceManager.h"
#include "VulkanTypes.h"
#include "CommandManager.h"
#include "VulkanSwapchain.h"
#include "VulkanTexture.h"
#include "Vertex.h"
#include "VulkanModel.h"
#include "Camera.h"
#include "ShaderUtils.h"
#include "RayTracingPipeline.h"
#include "GBufferPipeline.h"
#include "SSRQueryPipeline.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <chrono>
#include <fstream>

class Renderer
{

public:
    void init(VulkanContext &context, 
        ResourceManager &resourceManager, 
        CommandManager &commandManager, 
        VulkanSwapchain &swapchain, 
        VulkanModel &model, 
        Camera &camera,
        RayTracingPipeline &rtPipeline,
        GBufferPipeline &gbufferPipeline,
        SSRQueryPipeline &ssrQueryPipeline,
        const std::vector<glm::mat4>& modelMatrices,
        GLFWwindow* window);
        

    void drawFrame();
    void cleanup();

private:
    VulkanContext *context = nullptr;
    ResourceManager *resourceManager = nullptr;
    CommandManager *commandManager = nullptr;
    VulkanSwapchain *swapchain = nullptr;
    VulkanModel* model = nullptr;
    Camera* camera = nullptr;
    GLFWwindow* window = nullptr;
    RayTracingPipeline* rtPipeline = nullptr;
    GBufferPipeline* gbufferPipeline = nullptr;
    SSRQueryPipeline* ssrQueryPipeline = nullptr;
    std::vector<glm::mat4> modelMatrices;
    

    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout uboSetLayout;
    VkDescriptorSetLayout materialSetLayout;
    std::vector<VkDescriptorSet> uboDescriptorSets;
    std::vector<VkDescriptorSet> materialDescriptorSets;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    uint32_t currentFrame = 0;
    uint64_t frameNumber = 0;
    VkSemaphore timelineSemaphore;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorPool imguiDescriptorPool;

    VkDescriptorSet positionDebugTexture;
    VkDescriptorSet normalDebugTexture;
    VkDescriptorSet albedoDebugTexture;
    VkDescriptorSet ssrOutputDebugTexture;


    void renderImGuiOverlay(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createSyncObjects();
    void setupImgui();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void updateUniformBuffer(uint32_t currentImage);

};