#include "LensModel.h"
#include <fstream>
#include <vector>

void LensModel::init(VulkanContext& context, ResourceManager& resourceManager, const std::string& weightsPath)
{
    this->context = &context;
    this->resourceManager = &resourceManager;

    loadWeights(weightsPath);
    createNormConstants();
}

void LensModel::loadWeights(const std::string& weightsPath)
{
    std::cout << "Loading lens model weights from: " << weightsPath << std::endl;
    std::ifstream file(weightsPath, std::ios::binary | std::ios::ate);
    if(!file.is_open())
    {
        throw std::runtime_error("failed to open lens model weights file!");
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    if(fileSize != 4448 * sizeof(float))
    {
        throw std::runtime_error("lens weights file is not the expected 17792 bytes: " + weightsPath);
    }

    std::vector<float> weights(4448);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(weights.data()), fileSize);
    file.close();

    resourceManager->createBuffer(fileSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, weightsBuffer, weightsMemory);
    
    void* mapped;
    vkMapMemory(context->device, weightsMemory, 0, fileSize, 0, &mapped);
    memcpy(mapped, weights.data(), fileSize);
    vkUnmapMemory(context->device, weightsMemory);
}

void LensModel::createNormConstants()
{
    struct NormConstants
    {
        float positionScaleI;
        float positionScaleO;
        float wavelengthMin;
        float wavelengthMax;
        float intensityMin;
        float intensityMax;
        float pad0;
        float pad1;
    };

    VkDeviceSize bufferSize = sizeof(NormConstants);
    resourceManager->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, normConstantsBuffer, normConstantsMemory);
    vkMapMemory(context->device, normConstantsMemory, 0, bufferSize, 0, &normConstantsMapped);

    NormConstants norm{};
    norm.positionScaleI = 11.8579998;
    norm.positionScaleO = 11.8579998f / 1000.0f;
    norm.wavelengthMin = 357.3471984863281f;
    norm.wavelengthMax = 795.9625854492188f;
    norm.intensityMin = 0.4363940954208374f;
    norm.intensityMax = 0.5152989625930786f;

    memcpy(normConstantsMapped, &norm, bufferSize);
}

void LensModel::cleanup()
{
    vkDestroyBuffer(context->device, weightsBuffer, nullptr);
    vkFreeMemory(context->device, weightsMemory, nullptr);
    vkDestroyBuffer(context->device, normConstantsBuffer, nullptr);
    vkFreeMemory(context->device, normConstantsMemory, nullptr);
}
