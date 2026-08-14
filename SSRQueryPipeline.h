#pragma once
#define GLFW_INCLUDE_VULKAN

#include "VulkanContext.h"
#include "ResourceManager.h"
#include "CommandManager.h"
#include "VulkanSwapchain.h"
#include "GBufferPipeline.h"
#include "Camera.h"
#include "ShaderUtils.h"
#include "LensModel.h"

struct SSRQueryUBO {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewInverse;
    glm::mat4 projInverse;
    float maxDistance;
    float stepSize;
    int imageWidth;
    int imageHeight;
    glm::vec2 apertureOffset;
    float handoffPlaneDistance;
    float sensorPlaneDistance;
    float testWavelengthNm;
    float goldenTestX;
    float goldenTestZ;
    float goldenTestDirX;
    float goldenTestDirY;
    float goldenTestDirZ;
    float goldenTestWavelength;
    int visualizeFold;
    float foldEpsilon;
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

struct GoldenTestResult {
    float x_o;
    float z_o;
    float dirx_o;
    float diry_o;
    float dirz_o;
    float intensity;
    float pad0;
    float pad1;
};

class SSRQueryPipeline {
public:
    void init(VulkanContext& context, ResourceManager& resourceManager, CommandManager& commandManager,
              VulkanSwapchain& swapchain, GBufferPipeline& gbufferPipeline, Camera& camera, LensModel& lensModel);
    void updateQuery();
    void recordCommandBuffer(VkCommandBuffer commandBuffer);
    SSRQueryResult getResult();
    void cleanup();

    VkImageView ssrOutputImageView;
    VkSampler ssrOutputSampler;

    void setGoldenTestInput(float x, float z, float dirX, float dirY, float dirZ, float wavelength);
    GoldenTestResult getGoldenTestResult();

    void setApertureOffset(float x, float z);
    void setSensorPlaneDistance(float distance);
    void setVisualizeFold(bool enabled, float epsilon);

private:
    VulkanContext* context = nullptr;
    ResourceManager* resourceManager = nullptr;
    CommandManager* commandManager = nullptr;
    VulkanSwapchain* swapchain = nullptr;
    GBufferPipeline* gbufferPipeline = nullptr;
    Camera* camera = nullptr;
    LensModel* lensModel = nullptr;

    VkDescriptorSetLayout gbufferSetLayout;
    VkDescriptorSetLayout querySetLayout;
    VkDescriptorSetLayout lensSetLayout;

    VkDescriptorPool descriptorPool;
    
    VkDescriptorSet gbufferDescriptorSet;
    VkDescriptorSet queryDescriptorSet;
    VkDescriptorSet lensDescriptorSet;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    VkBuffer queryUboBuffer;
    VkDeviceMemory queryUboMemory;
    void* queryUboMapped;

    VkBuffer resultBuffer;
    VkDeviceMemory resultMemory;
    void* resultMapped;

    VkImage ssrOutputImage;
    VkDeviceMemory ssrOutputImageMemory;

    VkBuffer goldenTestResultBuffer;
    VkDeviceMemory goldenTestResultMemory;
    void* goldenTestResultMapped;

    float goldenTestX = 0.0f;
    float goldenTestZ = 0.0f;
    float goldenTestDirX = 0.0f;
    float goldenTestDirY = 1.0f;
    float goldenTestDirZ = 0.0f;
    float goldenTestWavelength = 550.0f;

    float apertureOffsetX = 0.0f;
    float apertureOffsetZ = 0.0f;
    float sensorPlaneDistance = 8.0f;
    bool visualizeFold = false;
    float foldEpsilon = 0.03f;

    void createDescriptorSetLayouts();
    void createPipeline();
    void createBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createOutputImage();
};