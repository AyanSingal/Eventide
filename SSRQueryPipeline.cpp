#include "SSRQueryPipeline.h"

void SSRQueryPipeline::init(VulkanContext& context, ResourceManager& resourceManager, CommandManager& commandManager,
              VulkanSwapchain& swapchain, GBufferPipeline& gbufferPipeline, Camera& camera, LensModel& lensModel)
{
    this->context = &context;
    this->resourceManager = &resourceManager;
    this->commandManager = &commandManager;
    this->swapchain = &swapchain;
    this->gbufferPipeline = &gbufferPipeline;
    this->camera = &camera;
    this->lensModel = &lensModel;
    createDescriptorSetLayouts();
    createPipeline();
    createBuffers();
    createOutputImage();
    createDescriptorPool();
    createDescriptorSets();
}

void SSRQueryPipeline::createDescriptorSetLayouts()
{
    std::array<VkDescriptorSetLayoutBinding, 4> gBufferBindings{};
    for(uint32_t i = 0; i < 4; i++)
    {
        gBufferBindings[i].binding = i;
        gBufferBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        gBufferBindings[i].descriptorCount = 1;
        gBufferBindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutCreateInfo gBufferLayoutInfo{};
    gBufferLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    gBufferLayoutInfo.bindingCount = static_cast<uint32_t>(gBufferBindings.size());
    gBufferLayoutInfo.pBindings = gBufferBindings.data();

    if(vkCreateDescriptorSetLayout(context->device, &gBufferLayoutInfo, nullptr, &gbufferSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create ssr gbuffer descriptor set layout!");
    }

    std::array<VkDescriptorSetLayoutBinding, 4> queryBindings{};
    queryBindings[0].binding = 0;
    queryBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    queryBindings[0].descriptorCount = 1;
    queryBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    queryBindings[1].binding = 1;
    queryBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    queryBindings[1].descriptorCount = 1;
    queryBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    queryBindings[2].binding = 2;
    queryBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    queryBindings[2].descriptorCount = 1;
    queryBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    queryBindings[3].binding = 3;
    queryBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    queryBindings[3].descriptorCount = 1;
    queryBindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo queryLayoutInfo{};
    queryLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    queryLayoutInfo.bindingCount = static_cast<uint32_t>(queryBindings.size());
    queryLayoutInfo.pBindings = queryBindings.data();

    if(vkCreateDescriptorSetLayout(context->device, &queryLayoutInfo, nullptr, &querySetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create ssr query descriptor set layout!");
    }

    std::array<VkDescriptorSetLayoutBinding, 2> lensBindings{};
    lensBindings[0].binding = 0;
    lensBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    lensBindings[0].descriptorCount = 1;
    lensBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    lensBindings[1].binding = 1;
    lensBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lensBindings[1].descriptorCount = 1;
    lensBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo lensLayoutInfo{};
    lensLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    lensLayoutInfo.bindingCount = static_cast<uint32_t>(lensBindings.size());
    lensLayoutInfo.pBindings = lensBindings.data();

    if(vkCreateDescriptorSetLayout(context->device, &lensLayoutInfo, nullptr, &lensSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create lens descriptor set layout!");
    }
}

void SSRQueryPipeline::createPipeline()
{
    auto compCode = ShaderUtils::readFile("shaders/ssr_query.comp.spv");
    VkShaderModule compModule = ShaderUtils::createShaderModule(context->device, compCode);

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = compModule;
    stageInfo.pName = "main";

    VkDescriptorSetLayout setLayouts[3] = { gbufferSetLayout, querySetLayout, lensSetLayout };

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 3;
    layoutInfo.pSetLayouts = setLayouts;

    if(vkCreatePipelineLayout(context->device, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create ssr query pipeline layout!");
    }

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = pipelineLayout;

    if(vkCreateComputePipelines(context->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create ssr query compute pipeline!");
    }

    vkDestroyShaderModule(context->device, compModule, nullptr);
}

void SSRQueryPipeline::createBuffers()
{
    resourceManager->createBuffer(sizeof(SSRQueryUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, queryUboBuffer, queryUboMemory);
    vkMapMemory(context->device, queryUboMemory, 0, sizeof(SSRQueryUBO), 0, &queryUboMapped);

    resourceManager->createBuffer(sizeof(SSRQueryResult), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, resultBuffer, resultMemory);
    vkMapMemory(context->device, resultMemory, 0, sizeof(SSRQueryResult), 0, &resultMapped);

    resourceManager->createBuffer(sizeof(GoldenTestResult), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, goldenTestResultBuffer, goldenTestResultMemory);
    vkMapMemory(context->device, goldenTestResultMemory, 0, sizeof(GoldenTestResult), 0, &goldenTestResultMapped);
}

void SSRQueryPipeline::createOutputImage()
{
    uint32_t width = swapchain->swapChainExtent.width;
    uint32_t height = swapchain->swapChainExtent.height;

    resourceManager->createImage(width, height,1 , VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        ssrOutputImage, ssrOutputImageMemory);
    ssrOutputImageView = resourceManager->createImageView(ssrOutputImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    resourceManager->transitionImageLayout(ssrOutputImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, 1);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    if(vkCreateSampler(context->device, &samplerInfo, nullptr, &ssrOutputSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create ssr output sampler!");
    }
}

void SSRQueryPipeline::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 4> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount = 4;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = 2;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = 3;
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[3].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 3;

    if(vkCreateDescriptorPool(context->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create ssr query descriptor pool!");
    }
}

void SSRQueryPipeline::createDescriptorSets()
{
    VkDescriptorSetAllocateInfo gbufferAllocInfo{};
    gbufferAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    gbufferAllocInfo.descriptorPool = descriptorPool;
    gbufferAllocInfo.descriptorSetCount = 1;
    gbufferAllocInfo.pSetLayouts = &gbufferSetLayout;

    if(vkAllocateDescriptorSets(context->device, &gbufferAllocInfo, &gbufferDescriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate ssr gbuffer descriptor set!");
    }

    std::array<VkDescriptorImageInfo, 4> imageInfos{};
    imageInfos[0] = { gbufferPipeline->gbufferSampler, gbufferPipeline->positionImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    imageInfos[1] = { gbufferPipeline->gbufferSampler, gbufferPipeline->normalImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    imageInfos[2] = { gbufferPipeline->albedoLinearSampler, gbufferPipeline->albedoImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    imageInfos[3] = { gbufferPipeline->gbufferSampler, gbufferPipeline->depthImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

    std::array<VkWriteDescriptorSet, 4> gbufferWrites{};
    for(uint32_t i = 0; i < 4; i++)
    {
        gbufferWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        gbufferWrites[i].dstSet = gbufferDescriptorSet;
        gbufferWrites[i].dstBinding = i;
        gbufferWrites[i].dstArrayElement = 0;
        gbufferWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        gbufferWrites[i].descriptorCount = 1;
        gbufferWrites[i].pImageInfo = &imageInfos[i];
    }
    vkUpdateDescriptorSets(context->device, static_cast<uint32_t>(gbufferWrites.size()), gbufferWrites.data(), 0, nullptr);

    VkDescriptorSetAllocateInfo queryAllocInfo{};
    queryAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    queryAllocInfo.descriptorPool = descriptorPool;
    queryAllocInfo.descriptorSetCount = 1;
    queryAllocInfo.pSetLayouts = &querySetLayout;

    if(vkAllocateDescriptorSets(context->device, &queryAllocInfo, &queryDescriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate ssr query descriptor set!");
    }

    VkDescriptorBufferInfo uboInfo{};
    uboInfo.buffer = queryUboBuffer;
    uboInfo.offset = 0;
    uboInfo.range = sizeof(SSRQueryUBO);

    VkDescriptorBufferInfo resultInfo{};
    resultInfo.buffer = resultBuffer;
    resultInfo.offset = 0;
    resultInfo.range = sizeof(SSRQueryResult);

    VkDescriptorImageInfo outputImageInfo{};
    outputImageInfo.imageView = ssrOutputImageView;
    outputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkDescriptorBufferInfo goldenResultInfo{};
    goldenResultInfo.buffer = goldenTestResultBuffer;
    goldenResultInfo.offset = 0;
    goldenResultInfo.range = sizeof(GoldenTestResult);


    std::array<VkWriteDescriptorSet, 4> queryWrites{};
    queryWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    queryWrites[0].dstSet = queryDescriptorSet;
    queryWrites[0].dstBinding = 0;
    queryWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    queryWrites[0].descriptorCount = 1;
    queryWrites[0].pBufferInfo = &uboInfo;

    queryWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    queryWrites[1].dstSet = queryDescriptorSet;
    queryWrites[1].dstBinding = 1;
    queryWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    queryWrites[1].descriptorCount = 1;
    queryWrites[1].pBufferInfo = &resultInfo;
    
    queryWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    queryWrites[2].dstSet = queryDescriptorSet;
    queryWrites[2].dstBinding = 2;
    queryWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    queryWrites[2].descriptorCount = 1;
    queryWrites[2].pImageInfo = &outputImageInfo;
    
    queryWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    queryWrites[3].dstSet = queryDescriptorSet;
    queryWrites[3].dstBinding = 3;
    queryWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    queryWrites[3].descriptorCount = 1;
    queryWrites[3].pBufferInfo = &goldenResultInfo;

    vkUpdateDescriptorSets(context->device, static_cast<uint32_t>(queryWrites.size()), queryWrites.data(), 0, nullptr);


    VkDescriptorSetAllocateInfo lensAllocInfo{};
    lensAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    lensAllocInfo.descriptorPool = descriptorPool;
    lensAllocInfo.descriptorSetCount = 1;
    lensAllocInfo.pSetLayouts = &lensSetLayout;

    if(vkAllocateDescriptorSets(context->device, &lensAllocInfo, &lensDescriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate lens descriptor set!");
    }

    VkDescriptorBufferInfo weightsInfo{};
    weightsInfo.buffer = lensModel->weightsBuffer;
    weightsInfo.offset = 0;
    weightsInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo normInfo{};
    normInfo.buffer = lensModel->normConstantsBuffer;
    normInfo.offset = 0;
    normInfo.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 2> lensWrites{};
    lensWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    lensWrites[0].dstSet = lensDescriptorSet;
    lensWrites[0].dstBinding = 0;
    lensWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    lensWrites[0].descriptorCount = 1;
    lensWrites[0].pBufferInfo = &weightsInfo;

    lensWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    lensWrites[1].dstSet = lensDescriptorSet;
    lensWrites[1].dstBinding = 1;
    lensWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lensWrites[1].descriptorCount = 1;
    lensWrites[1].pBufferInfo = &normInfo;

    vkUpdateDescriptorSets(context->device, static_cast<uint32_t>(lensWrites.size()), lensWrites.data(), 0, nullptr);
}

void SSRQueryPipeline::updateQuery()
{
    float aspectRatio = swapchain->swapChainExtent.width / (float)swapchain->swapChainExtent.height;

    glm::mat4 view = camera->getViewMatrix();
    glm::mat4 proj = camera->getProjectionMatrix(aspectRatio);

    SSRQueryUBO ubo{};
    ubo.view = view;
    ubo.proj = proj;
    ubo.viewInverse = glm::inverse(view);
    ubo.projInverse = glm::inverse(proj);
    ubo.maxDistance = 20.0f;
    ubo.stepSize = 0.05f;
    ubo.imageWidth = swapchain->swapChainExtent.width;
    ubo.imageHeight = swapchain->swapChainExtent.height;
    ubo.apertureOffset = glm::vec2(apertureOffsetX, apertureOffsetZ);
    ubo.handoffPlaneDistance = 13.17f / 1000.0f;
    ubo.sensorPlaneDistance = sensorPlaneDistance;
    ubo.testWavelengthNm = 550.0f;

    ubo.goldenTestX = goldenTestX;
    ubo.goldenTestZ = goldenTestZ;
    ubo.goldenTestDirX = goldenTestDirX;
    ubo.goldenTestDirY = goldenTestDirY;
    ubo.goldenTestDirZ = goldenTestDirZ;
    ubo.goldenTestWavelength = goldenTestWavelength;

    ubo.visualizeFold = visualizeFold ? 1 : 0;
    ubo.foldEpsilon = foldEpsilon;

    memcpy(queryUboMapped, &ubo, sizeof(ubo));
}

void SSRQueryPipeline::setApertureOffset(float x, float z)
{
    apertureOffsetX = x;
    apertureOffsetZ = z;
}

void SSRQueryPipeline::setSensorPlaneDistance(float distance)
{
    sensorPlaneDistance = distance;
}

void SSRQueryPipeline::setVisualizeFold(bool enabled, float epsilon)
{
    visualizeFold = enabled;
    foldEpsilon = epsilon;
}

void SSRQueryPipeline::setGoldenTestInput(float x, float z, float dirX, float dirY, float dirZ, float wavelength)
{
    goldenTestX = x;
    goldenTestZ = z;
    goldenTestDirX = dirX;
    goldenTestDirY = dirY;
    goldenTestDirZ = dirZ;
    goldenTestWavelength = wavelength;
}

GoldenTestResult SSRQueryPipeline::getGoldenTestResult()
{
    GoldenTestResult result;
    memcpy(&result, goldenTestResultMapped, sizeof(result));
    return result;
}

void SSRQueryPipeline::recordCommandBuffer(VkCommandBuffer commandBuffer)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    VkDescriptorSet sets[3] = { gbufferDescriptorSet, queryDescriptorSet, lensDescriptorSet };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 3, sets, 0, nullptr);

    uint32_t groupsX = (swapchain->swapChainExtent.width + 15) / 16;
    uint32_t groupsY = (swapchain->swapChainExtent.height + 15) / 16;
    vkCmdDispatch(commandBuffer, groupsX, groupsY, 1);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.image = ssrOutputImage;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);
}


SSRQueryResult SSRQueryPipeline::getResult()
{
    SSRQueryResult result;
    memcpy(&result, resultMapped, sizeof(result));
    return result;
}

void SSRQueryPipeline::cleanup()
{
    vkDestroyBuffer(context->device, queryUboBuffer, nullptr);
    vkFreeMemory(context->device, queryUboMemory, nullptr);
    vkDestroyBuffer(context->device, resultBuffer, nullptr);
    vkFreeMemory(context->device, resultMemory, nullptr);

    vkDestroyDescriptorPool(context->device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(context->device, gbufferSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(context->device, querySetLayout, nullptr);
    vkDestroyDescriptorSetLayout(context->device, lensSetLayout, nullptr);

    vkDestroySampler(context->device, ssrOutputSampler, nullptr);
    vkDestroyImageView(context->device, ssrOutputImageView, nullptr);
    vkDestroyImage(context->device, ssrOutputImage, nullptr);
    vkFreeMemory(context->device, ssrOutputImageMemory, nullptr);

    vkDestroyBuffer(context->device, goldenTestResultBuffer, nullptr);
    vkFreeMemory(context->device, goldenTestResultMemory, nullptr);

    vkDestroyPipeline(context->device, pipeline, nullptr);
    vkDestroyPipelineLayout(context->device, pipelineLayout, nullptr);
}