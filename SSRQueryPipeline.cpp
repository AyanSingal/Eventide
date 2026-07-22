#include "SSRQueryPipeline.h"

void SSRQueryPipeline::init(VulkanContext& context, ResourceManager& resourceManager, CommandManager& commandManager,
              VulkanSwapchain& swapchain, GBufferPipeline& gbufferPipeline, Camera& camera)
{
    this->context = &context;
    this->resourceManager = &resourceManager;
    this->commandManager = &commandManager;
    this->swapchain = &swapchain;
    this->gbufferPipeline = &gbufferPipeline;
    this->camera = &camera;

    createDescriptorSetLayouts();
    createPipeline();
    createBuffers();
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

    std::array<VkDescriptorSetLayoutBinding, 2> queryBindings{};
    queryBindings[0].binding = 0;
    queryBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    queryBindings[0].descriptorCount = 1;
    queryBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    queryBindings[1].binding = 1;
    queryBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    queryBindings[1].descriptorCount = 1;
    queryBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo queryLayoutInfo{};
    queryLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    queryLayoutInfo.bindingCount = static_cast<uint32_t>(queryBindings.size());
    queryLayoutInfo.pBindings = queryBindings.data();

    if(vkCreateDescriptorSetLayout(context->device, &queryLayoutInfo, nullptr, &querySetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create ssr query descriptor set layout!");
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

    VkDescriptorSetLayout setLayouts[2] = { gbufferSetLayout, querySetLayout };

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 2;
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
}

void SSRQueryPipeline::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount = 4;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = 1;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 2;

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
    imageInfos[2] = { gbufferPipeline->gbufferSampler, gbufferPipeline->albedoImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
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

    std::array<VkWriteDescriptorSet, 2> queryWrites{};
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

    vkUpdateDescriptorSets(context->device, static_cast<uint32_t>(queryWrites.size()), queryWrites.data(), 0, nullptr);
}

void SSRQueryPipeline::updateQuery()
{
    float aspectRatio = swapchain->swapChainExtent.width / (float)swapchain->swapChainExtent.height;

    glm::mat4 view = camera->getViewMatrix();
    glm::vec3 forward = glm::normalize(glm::vec3(glm::inverse(view) * glm::vec4(0, 0, -1, 0)));

    SSRQueryUBO ubo{};
    ubo.rayOrigin = glm::vec4(camera->position, 1.0f);
    ubo.rayDirection = glm::vec4(forward, 0.0f);
    ubo.view = view;
    ubo.proj = camera->getProjectionMatrix(aspectRatio);
    ubo.maxDistance = 20.0f;
    ubo.stepSize = 0.05f;

    memcpy(queryUboMapped, &ubo, sizeof(ubo));
}

void SSRQueryPipeline::recordCommandBuffer(VkCommandBuffer commandBuffer)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    VkDescriptorSet sets[2] = { gbufferDescriptorSet, queryDescriptorSet };
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 2, sets, 0, nullptr);

    vkCmdDispatch(commandBuffer, 1, 1, 1);
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

    vkDestroyPipeline(context->device, pipeline, nullptr);
    vkDestroyPipelineLayout(context->device, pipelineLayout, nullptr);
}