#include "RayTracingPipeline.h"

void RayTracingPipeline::createStorageImage()
{
    resourceManager->createImage(
        swapchain->swapChainExtent.width,
        swapchain->swapChainExtent.height,
        1, VK_SAMPLE_COUNT_1_BIT,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        storageImage, storageImageMemory);

    storageImageView = resourceManager->createImageView(
        storageImage,
        VK_FORMAT_R8G8B8_UNORM,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1
    );

    resourceManager->transitionImageLayout(
        storageImage, VK_FORMAT_R8G8B8_UNORM,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 1);
}


void RayTracingPipeline::createUBO()
{
    VkDeviceSize bufferSize = sizeof(RTUniformBufferObject);
    resourceManager->createBuffer(bufferSize, 
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        uboBuffer, uboMemory);
    vkMapMemory(context->device, uboMemory, 0, bufferSize, 0, &uboMapped);
}

void RayTracingPipeline::updateUBO()
{
    float aspectRatio = swapchain->swapChainExtent.width / (float)swapchain->swapChainExtent.height;

    RTUniformBufferObject ubo{};
    ubo.viewInverse = glm::inverse(camera->getViewMatrix());
    ubo.projInverse = glm::inverse(camera->getProjectionMatrix(aspectRatio));

    memcpy(uboMapped, &ubo, sizeof(ubo));
}

void RayTracingPipeline::createDescriptorSets()
{
    //LAYOUT
    VkDescriptorSetLayoutBinding tlasBinding{};
    tlasBinding.binding = 0;
    tlasBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    tlasBinding.descriptorCount = 1;
    tlasBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    
    VkDescriptorSetLayoutBinding imageBinding{};
    imageBinding.binding = 1;
    imageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    imageBinding.descriptorCount = 1;
    imageBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 2;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    std::array<VkDescriptorSetLayoutBinding, 3> bindings = { tlasBinding, imageBinding, uboBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if(vkCreateDescriptorSetLayout(context->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create RT descriptor set layout!");
    }


    //POOL
    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[1].descriptorCount = 1;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[2].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;

    if(vkCreateDescriptorPool(context->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    if(vkAllocateDescriptorSets(context->device, &allocInfo, &descriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate RT descriptor set!");
    }


    VkWriteDescriptorSetAccelerationStructureKHR tlasInfo{};
    tlasInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    tlasInfo.accelerationStructureCount = 1;
    tlasInfo.pAccelerationStructures = &rtAS->tlas.tlasHandle;

    VkWriteDescriptorSet tlasWrite{};
    tlasWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    tlasWrite.pNext = &tlasInfo;
    tlasWrite.dstSet = descriptorSet;
    tlasWrite.dstBinding = 0;
    tlasWrite.descriptorCount = 1;
    tlasWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    VkDescriptorImageInfo storageImageInfo{};
    storageImageInfo.imageView = storageImageView;
    storageImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet imageWrite{};
    imageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    imageWrite.dstSet = descriptorSet;
    imageWrite.dstBinding = 1;
    imageWrite.descriptorCount = 1;
    imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    imageWrite.pImageInfo = &storageImageInfo;

    VkDescriptorBufferInfo uboInfo{};
    uboInfo.buffer = uboBuffer;
    uboInfo.offset = 0;
    uboInfo.range = sizeof(RTUniformBufferObject);

    VkWriteDescriptorSet uboWrite{};
    uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    uboWrite.dstSet = descriptorSet;
    uboWrite.dstBinding = 2;
    uboWrite.descriptorCount = 1;
    uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboWrite.pBufferInfo = &uboInfo;

    std::array<VkWriteDescriptorSet, 3> writes = {tlasWrite, imageWrite, uboWrite};
    vkUpdateDescriptorSets(context->device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void RayTracingPipeline::createPipeline()
{
    auto raygenCode = ShaderUtils::readFile("shaders/rgen.spv");
    auto missCode = ShaderUtils::readFile("shaders/rmiss.spv");
    auto chitCode = ShaderUtils::readFile("shaders/rchit.spv");

    VkShaderModule raygenModule = ShaderUtils::createShaderModule(context->device, raygenCode);
    VkShaderModule missModule = ShaderUtils::createShaderModule(context->device, missCode);
    VkShaderModule chitModule = ShaderUtils::createShaderModule(context->device, chitCode);

    VkPipelineShaderStageCreateInfo raygenStage{};
    raygenStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    raygenStage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    raygenStage.module = raygenModule;
    raygenStage.pName = "main";

    VkPipelineShaderStageCreateInfo missStage{};
    missStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    missStage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    missStage.module = missModule;
    missStage.pName = "main";

    VkPipelineShaderStageCreateInfo chitStage{};
    chitStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    chitStage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    chitStage.module = chitModule;
    chitStage.pName = "main";

    std::array<VkPipelineShaderStageCreateInfo, 3> stages = { raygenStage, missStage, chitStage };

    VkRayTracingShaderGroupCreateInfoKHR raygenGroup{};
    raygenGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    raygenGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    raygenGroup.generalShader = 0; //Index into the stages array above
    raygenGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
    raygenGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    raygenGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

    VkRayTracingShaderGroupCreateInfoKHR missGroup{};
    missGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    missGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    missGroup.generalShader = 1;
    missGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
    missGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    missGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

    VkRayTracingShaderGroupCreateInfoKHR hitGroup{};
    hitGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    hitGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    hitGroup.generalShader = VK_SHADER_UNUSED_KHR;
    hitGroup.closestHitShader = 2;
    hitGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    hitGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

    std::array<VkRayTracingShaderGroupCreateInfoKHR, 3> groups = {raygenGroup, missGroup, hitGroup};
}