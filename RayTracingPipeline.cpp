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
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1
    );

    resourceManager->transitionImageLayout(
        storageImage, VK_FORMAT_R8G8B8A8_UNORM,
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

void RayTracingPipeline::createQueryResultBuffer()
{
    VkDeviceSize bufferSize = sizeof(RTQueryResult);
    resourceManager->createBuffer(bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        rtQueryResultBuffer, rtQueryResultMemory);
    vkMapMemory(context->device, rtQueryResultMemory, 0, bufferSize, 0, &rtQueryResultMapped);
}

RTQueryResult RayTracingPipeline::getQueryResult()
{
    RTQueryResult result;
    memcpy(&result, rtQueryResultMapped, sizeof(RTQueryResult));
    return result;
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
    uint32_t meshCount = static_cast<uint32_t>(model->subMeshes.size());
    uint32_t texCount = static_cast<uint32_t>(model->textures.size());

    //LAYOUT
    VkDescriptorSetLayoutBinding tlasBinding{};
    tlasBinding.binding = 0;
    tlasBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    tlasBinding.descriptorCount = 1;
    tlasBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    
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

    VkDescriptorSetLayoutBinding vertexBinding{};
    vertexBinding.binding = 3;
    vertexBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vertexBinding.descriptorCount = meshCount;
    vertexBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    VkDescriptorSetLayoutBinding indexBinding{};
    indexBinding.binding = 4;
    indexBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    indexBinding.descriptorCount = meshCount;
    indexBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    VkDescriptorSetLayoutBinding textureBinding{};
    textureBinding.binding = 5;
    textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureBinding.descriptorCount = texCount;
    textureBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    VkDescriptorSetLayoutBinding resultBinding{};
    resultBinding.binding = 6;
    resultBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    resultBinding.descriptorCount = 1;
    resultBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    std::array<VkDescriptorSetLayoutBinding, 7> bindings = { tlasBinding, imageBinding, uboBinding, vertexBinding, indexBinding, textureBinding, resultBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if(vkCreateDescriptorSetLayout(context->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create RT descriptor set layout!");
    }


    //POOL
    std::array<VkDescriptorPoolSize, 6> poolSizes{};
    poolSizes[0] = {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1};
    poolSizes[1] = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1};
    poolSizes[2] = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1};
    poolSizes[3] = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, meshCount * 2};
    poolSizes[4] = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texCount};
    poolSizes[5] = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1};

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;

    if(vkCreateDescriptorPool(context->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create RT descriptor pool!");
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

    std::vector<VkDescriptorBufferInfo> vertexInfos(meshCount);
    for(uint32_t i = 0; i < meshCount; i++){
        vertexInfos[i].buffer = model->subMeshes[i].vertexBuffer;
        vertexInfos[i].offset = 0;
        vertexInfos[i].range = VK_WHOLE_SIZE;
    }

    VkWriteDescriptorSet vertexWrite{};
    vertexWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vertexWrite.dstSet = descriptorSet;
    vertexWrite.dstBinding = 3;
    vertexWrite.descriptorCount = meshCount;
    vertexWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vertexWrite.pBufferInfo = vertexInfos.data();

    std::vector<VkDescriptorBufferInfo> indexInfos(meshCount);
    for(uint32_t i = 0; i < meshCount; i++){
        indexInfos[i].buffer = model->subMeshes[i].indexBuffer;
        indexInfos[i].offset = 0;
        indexInfos[i].range = VK_WHOLE_SIZE;
    }

    VkWriteDescriptorSet indexWrite{};
    indexWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    indexWrite.dstSet = descriptorSet;
    indexWrite.dstBinding = 4;
    indexWrite.descriptorCount = meshCount;
    indexWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    indexWrite.pBufferInfo = indexInfos.data();

    std::vector<VkDescriptorImageInfo> textureInfos(texCount);
    for(uint32_t i = 0; i < texCount; i++){
        textureInfos[i].imageView = model->textures[i].textureImageView;
        textureInfos[i].sampler = model->textures[i].textureSampler;
        textureInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkWriteDescriptorSet textureWrite{};
    textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    textureWrite.dstSet = descriptorSet;
    textureWrite.dstBinding = 5;
    textureWrite.descriptorCount = texCount;
    textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureWrite.pImageInfo = textureInfos.data();

    VkDescriptorBufferInfo resultInfo{};
    resultInfo.buffer = rtQueryResultBuffer;
    resultInfo.offset = 0;
    resultInfo.range = sizeof(RTQueryResult);

    VkWriteDescriptorSet resultWrite{};
    resultWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    resultWrite.dstSet = descriptorSet;
    resultWrite.dstBinding = 6;
    resultWrite.descriptorCount = 1;
    resultWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    resultWrite.pBufferInfo = &resultInfo;

    std::array<VkWriteDescriptorSet, 7> writes = {tlasWrite, imageWrite, uboWrite, vertexWrite, indexWrite, textureWrite, resultWrite};
    vkUpdateDescriptorSets(context->device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void RayTracingPipeline::createPipeline()
{
    auto raygenCode = ShaderUtils::readFile("shaders/raygen.rgen.spv");
    auto missCode = ShaderUtils::readFile("shaders/miss.rmiss.spv");
    auto chitCode = ShaderUtils::readFile("shaders/closesthit.rchit.spv");
    auto shadowMissCode = ShaderUtils::readFile("shaders/shadowmiss.rmiss.spv");

    VkShaderModule raygenModule = ShaderUtils::createShaderModule(context->device, raygenCode);
    VkShaderModule missModule = ShaderUtils::createShaderModule(context->device, missCode);
    VkShaderModule chitModule = ShaderUtils::createShaderModule(context->device, chitCode);
    VkShaderModule shadowMissModule = ShaderUtils::createShaderModule(context->device, shadowMissCode);

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

    VkPipelineShaderStageCreateInfo shadowMissStage{};
    shadowMissStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shadowMissStage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    shadowMissStage.module = shadowMissModule;
    shadowMissStage.pName = "main";

    std::array<VkPipelineShaderStageCreateInfo, 4> stages = { raygenStage, missStage, chitStage, shadowMissStage };

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

    VkRayTracingShaderGroupCreateInfoKHR shadowMissGroup{};
    shadowMissGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    shadowMissGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    shadowMissGroup.generalShader = 3;
    shadowMissGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
    shadowMissGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    shadowMissGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

    std::array<VkRayTracingShaderGroupCreateInfoKHR, 4> groups = {raygenGroup, missGroup, hitGroup, shadowMissGroup};

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descriptorSetLayout;

    if(vkCreatePipelineLayout(context->device, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create RT pipeline layout!");
    }

    VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
    pipelineInfo.pStages = stages.data();
    pipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
    pipelineInfo.pGroups = groups.data();
    pipelineInfo.maxPipelineRayRecursionDepth = 2;
    pipelineInfo.layout = pipelineLayout;

    if(context->vkCreateRayTracingPipelinesKHR(context->device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create ray tracing pipeline!");
    }

    vkDestroyShaderModule(context->device, raygenModule, nullptr);
    vkDestroyShaderModule(context->device, missModule, nullptr);
    vkDestroyShaderModule(context->device, chitModule, nullptr);
    vkDestroyShaderModule(context->device, shadowMissModule, nullptr);
}

void RayTracingPipeline::createShaderBindingTable()
{
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties{};
    rtProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

    VkPhysicalDeviceProperties2 deviceProps{};
    deviceProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProps.pNext = &rtProperties;
    vkGetPhysicalDeviceProperties2(context->physicalDevice, &deviceProps);

    uint32_t handleSize = rtProperties.shaderGroupHandleSize;
    uint32_t handleAlignment = rtProperties.shaderGroupHandleAlignment;
    uint32_t baseAlignment = rtProperties.shaderGroupBaseAlignment;

    uint32_t handleSizeAligned = (handleSize + handleAlignment - 1) & ~(handleAlignment - 1);

    uint32_t groupCount = 4;

    uint32_t sbtSize = groupCount * handleSize;
    std::vector<uint8_t> handleData(sbtSize);
    context->vkGetRayTracingShaderGroupHandlesKHR(
        context->device, pipeline, 0, groupCount, sbtSize, handleData.data());

    uint32_t raygenRegionSize = (handleSizeAligned + baseAlignment - 1) & ~(baseAlignment - 1);
    uint32_t missCount = 2;
    uint32_t missRegionSize = missCount * handleSizeAligned;
    missRegionSize = (missRegionSize + baseAlignment - 1) & ~(baseAlignment - 1);
    uint32_t hitRegionSize = (handleSizeAligned + baseAlignment - 1) & ~(baseAlignment - 1);
    uint32_t totalSize = raygenRegionSize + missRegionSize + hitRegionSize;

    resourceManager->createBuffer(totalSize,
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        sbtBuffer, sbtMemory);

    void* mapped;
    vkMapMemory(context->device, sbtMemory, 0, totalSize, 0, &mapped);
    uint8_t* dst = static_cast<uint8_t*>(mapped);

    memcpy(dst, handleData.data() + 0 * handleSize, handleSize);
    memcpy(dst + raygenRegionSize, handleData.data() + 1 * handleSize, handleSize);
    memcpy(dst + raygenRegionSize + handleSizeAligned, handleData.data() + 3 * handleSize, handleSize);
    memcpy(dst + raygenRegionSize + missRegionSize, handleData.data() + 2 * handleSize, handleSize);

    vkUnmapMemory(context->device, sbtMemory);

    VkBufferDeviceAddressInfo addrInfo{};
    addrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addrInfo.buffer = sbtBuffer;
    VkDeviceAddress sbtAddress = context->vkGetBufferDeviceAddressKHR(context->device, &addrInfo);

    raygenRegion.deviceAddress = sbtAddress;
    raygenRegion.stride = handleSizeAligned;
    raygenRegion.size = handleSizeAligned;

    missRegion.deviceAddress = sbtAddress + raygenRegionSize;
    missRegion.stride = handleSizeAligned;
    missRegion.size = missRegionSize;

    hitRegion.deviceAddress = sbtAddress + raygenRegionSize + missRegionSize;
    hitRegion.stride = handleSizeAligned;
    hitRegion.size = hitRegionSize;

    callableRegion = {};
}

void RayTracingPipeline::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                            pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

    context->vkCmdTraceRaysKHR(commandBuffer,
        &raygenRegion, &missRegion, &hitRegion, &callableRegion,
        swapchain->swapChainExtent.width,
        swapchain->swapChainExtent.height, 1);

    VkImageMemoryBarrier storageSrcBarrier{};
    storageSrcBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    storageSrcBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    storageSrcBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    storageSrcBarrier.image = storageImage;
    storageSrcBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    storageSrcBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    storageSrcBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    storageSrcBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    storageSrcBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    VkImageMemoryBarrier swapDstBarrier{};
    swapDstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    swapDstBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    swapDstBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    swapDstBarrier.image = swapchain->swapChainImages[imageIndex];
    swapDstBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    swapDstBarrier.srcAccessMask = 0;
    swapDstBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    swapDstBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    swapDstBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    std::array<VkImageMemoryBarrier, 2> preCopyBarriers = {storageSrcBarrier, swapDstBarrier};
    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr,
        static_cast<uint32_t>(preCopyBarriers.size()), preCopyBarriers.data());
    
    VkImageCopy copyRegion{};
    copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.extent = {swapchain->swapChainExtent.width, swapchain->swapChainExtent.height, 1};
    vkCmdCopyImage(commandBuffer,
        storageImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        swapchain->swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copyRegion);
    
    VkImageMemoryBarrier storageBackBarrier{};
    storageBackBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    storageBackBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    storageBackBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    storageBackBarrier.image = storageImage;
    storageBackBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    storageBackBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    storageBackBarrier.dstAccessMask = 0;
    storageBackBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    storageBackBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    VkImageMemoryBarrier swapPresentBarrier{};
    swapPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    swapPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    swapPresentBarrier.newLayout =  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    swapPresentBarrier.image = swapchain->swapChainImages[imageIndex];
    swapPresentBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    swapPresentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    swapPresentBarrier.dstAccessMask = 0;
    swapPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    swapPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    std::array<VkImageMemoryBarrier, 2> postCopyBarriers = {storageBackBarrier, swapPresentBarrier};
    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0, 0, nullptr, 0, nullptr,
        static_cast<uint32_t>(postCopyBarriers.size()), postCopyBarriers.data());
}

void RayTracingPipeline::init(VulkanContext& context, ResourceManager& resourceManager,
                                CommandManager& commandManager, RayTracingAS& rtAS,
                                VulkanSwapchain& swapchain, VulkanModel& model, Camera& camera)
{
    this->context = &context;
    this->resourceManager = &resourceManager;
    this->commandManager = &commandManager;
    this->rtAS = &rtAS;
    this->swapchain = &swapchain;
    this->camera = &camera;
    this->model = &model;

    createStorageImage();
    createUBO();
    createQueryResultBuffer();
    createDescriptorSets();
    createPipeline();
    createShaderBindingTable();
}

void RayTracingPipeline::cleanup()
{
    vkDestroyImageView(context->device, storageImageView, nullptr);
    vkDestroyImage(context->device, storageImage, nullptr);
    vkFreeMemory(context->device, storageImageMemory, nullptr);

    vkDestroyBuffer(context->device, uboBuffer, nullptr);
    vkFreeMemory(context->device, uboMemory, nullptr);

    vkDestroyBuffer(context->device, sbtBuffer, nullptr);
    vkFreeMemory(context->device, sbtMemory, nullptr);

    vkDestroyBuffer(context->device, rtQueryResultBuffer, nullptr);
    vkFreeMemory(context->device, rtQueryResultMemory, nullptr);

    vkDestroyPipeline(context->device, pipeline, nullptr);
    vkDestroyPipelineLayout(context->device, pipelineLayout, nullptr);
    vkDestroyDescriptorPool(context->device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(context->device, descriptorSetLayout, nullptr);
}