#include "GBufferPipeline.h"

void GBufferPipeline::init(VulkanContext& context, ResourceManager& resourceManager
    ,CommandManager& commandManager, VulkanSwapchain& swapchain, VulkanModel& model, Camera& camera, const std::vector<glm::mat4>& modelMatrices)
{
    this->context = &context;
    this->resourceManager = &resourceManager;
    this->commandManager = &commandManager;
    this->swapchain = &swapchain;
    this->model = &model;
    this->camera = &camera;
    this->modelMatrices = &modelMatrices;

    createGBufferImages();
    createSampler();
    createDescriptorSetLayout();
    createPipeline();
    createUniformBuffer();
    createDescriptorPool();
    createDescriptorSets();
}

void GBufferPipeline::createGBufferImages()
{
    uint32_t width = swapchain->swapChainExtent.width;
    uint32_t height = swapchain->swapChainExtent.height;

    resourceManager->createImage(width, height, 1, VK_SAMPLE_COUNT_1_BIT, positionFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        positionImage, positionImageMemory);
    positionImageView = resourceManager->createImageView(positionImage, positionFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    resourceManager->createImage(width, height, 1, VK_SAMPLE_COUNT_1_BIT, normalFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        normalImage, normalImageMemory);
    normalImageView = resourceManager->createImageView(normalImage, normalFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    resourceManager->createImage(width, height, 1, VK_SAMPLE_COUNT_1_BIT, albedoFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        albedoImage, albedoImageMemory);
    albedoImageView = resourceManager->createImageView(albedoImage, albedoFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    VkFormat depthFormat = context->findDepthFormat();
    resourceManager->createImage(width, height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        depthImage, depthImageMemory);
    depthImageView = resourceManager->createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void GBufferPipeline::createSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;

    if(vkCreateSampler(context->device, &samplerInfo, nullptr, &gbufferSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create gbuffer sampler!");
    }
}

void GBufferPipeline::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo uboLayoutInfo{};
    uboLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    uboLayoutInfo.bindingCount = 1;
    uboLayoutInfo.pBindings = &uboLayoutBinding;

    if(vkCreateDescriptorSetLayout(context->device, &uboLayoutInfo, nullptr, &uboSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create gbuffer ubo descriptor set layout!");
    }

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
    materialLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    materialLayoutInfo.bindingCount = 1;
    materialLayoutInfo.pBindings = &samplerLayoutBinding;

    if(vkCreateDescriptorSetLayout(context->device, &materialLayoutInfo, nullptr, &materialSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create gbuffer material descriptor set layout!");
    }
}

void GBufferPipeline::createPipeline()
{
    auto vertShaderCode = ShaderUtils::readFile("shaders/gbuffer.vert.spv");
    auto fragShaderCode = ShaderUtils::readFile("shaders/gbuffer.frag.spv");

    VkShaderModule vertShaderModule = ShaderUtils::createShaderModule(context->device, vertShaderCode);
    VkShaderModule fragShaderModule = ShaderUtils::createShaderModule(context->device, fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.sampleShadingEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    std::array<VkPipelineColorBlendAttachmentState, 3> colorBlendAttachments = {colorBlendAttachment, colorBlendAttachment, colorBlendAttachment};

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    colorBlending.pAttachments = colorBlendAttachments.data();

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);

    VkDescriptorSetLayout descriptorSetLayouts[] = { uboSetLayout, materialSetLayout };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 2;
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if(vkCreatePipelineLayout(context->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create gbuffer pipeline layout!");
    }

    std::array<VkFormat, 3> colorFormats = { positionFormat, normalFormat, albedoFormat };

    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
    pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingCreateInfo.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
    pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();
    pipelineRenderingCreateInfo.depthAttachmentFormat = context->findDepthFormat();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE;
    pipelineInfo.pNext = &pipelineRenderingCreateInfo;

    if(vkCreateGraphicsPipelines(context->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create gbuffer graphics pipeline!");
    }

    vkDestroyShaderModule(context->device, fragShaderModule, nullptr);
    vkDestroyShaderModule(context->device, vertShaderModule, nullptr);
}

void GBufferPipeline::createUniformBuffer()
{
    VkDeviceSize bufferSize = sizeof(GBufferUniformBufferObject);
    resourceManager->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uboBuffer, uboMemory);
    vkMapMemory(context->device, uboMemory, 0, bufferSize, 0, &uboMapped);
}

void GBufferPipeline::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(model->textures.size());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1 + static_cast<uint32_t>(model->textures.size());

    if(vkCreateDescriptorPool(context->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create gbuffer descriptor pool!");
    }
}

void GBufferPipeline::createDescriptorSets()
{
    VkDescriptorSetAllocateInfo uboAllocInfo{};
    uboAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    uboAllocInfo.descriptorPool = descriptorPool;
    uboAllocInfo.descriptorSetCount = 1;
    uboAllocInfo.pSetLayouts = &uboSetLayout;

    if(vkAllocateDescriptorSets(context->device, &uboAllocInfo, &uboDescriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate gbuffer ubo descriptor set!");
    }

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uboBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(GBufferUniformBufferObject);

    VkWriteDescriptorSet uboWrite{};
    uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    uboWrite.dstSet = uboDescriptorSet;
    uboWrite.dstBinding = 0;
    uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboWrite.descriptorCount = 1;
    uboWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(context->device, 1, &uboWrite, 0, nullptr);

    std::vector<VkDescriptorSetLayout> matLayouts(model->textures.size(), materialSetLayout);
    VkDescriptorSetAllocateInfo matAllocInfo{};
    matAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    matAllocInfo.descriptorPool = descriptorPool;
    matAllocInfo.descriptorSetCount = static_cast<uint32_t>(model->textures.size());
    matAllocInfo.pSetLayouts = matLayouts.data();
    materialDescriptorSets.resize(model->textures.size());

    if(vkAllocateDescriptorSets(context->device, &matAllocInfo, materialDescriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate gbuffer material descriptor sets!");
    }

    for(size_t i = 0; i < model->textures.size(); i++)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = model->textures[i].textureImageView;
        imageInfo.sampler = model->textures[i].textureSampler;

        VkWriteDescriptorSet matWrite{};
        matWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        matWrite.dstSet = materialDescriptorSets[i];
        matWrite.dstBinding = 0;
        matWrite.dstArrayElement = 0;
        matWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        matWrite.descriptorCount = 1;
        matWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(context->device, 1, &matWrite, 0, nullptr);
    }
}

void GBufferPipeline::updateUBO()
{
    float aspectRatio = static_cast<float>(swapchain->swapChainExtent.width) / static_cast<float>(swapchain->swapChainExtent.height);

    GBufferUniformBufferObject ubo{};
    ubo.view = camera->getViewMatrix();
    ubo.proj = camera->getProjectionMatrix(aspectRatio);

    memcpy(uboMapped, &ubo, sizeof(ubo));
}

void GBufferPipeline::recordCommandBuffer(VkCommandBuffer commandBuffer)
{
    VkImageLayout prevColorLayout = firstFrame ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkImageLayout prevDepthLayout = firstFrame ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    std::array<VkImage, 3> colorImages = { positionImage, normalImage, albedoImage };
    std::vector<VkImageMemoryBarrier> toAttachmentBarriers;

    for (VkImage image : colorImages)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = prevColorLayout;
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = firstFrame ? 0 : VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toAttachmentBarriers.push_back(barrier);
    }

    VkImageMemoryBarrier depthBarrier{};
    depthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    depthBarrier.oldLayout = prevDepthLayout;
    depthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthBarrier.image = depthImage;
    depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthBarrier.subresourceRange.levelCount = 1;
    depthBarrier.subresourceRange.layerCount = 1;
    depthBarrier.srcAccessMask = firstFrame ? 0 : VK_ACCESS_SHADER_READ_BIT;
    depthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    depthBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    depthBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toAttachmentBarriers.push_back(depthBarrier);

    vkCmdPipelineBarrier(commandBuffer,
                         firstFrame ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT),
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         0, 0, nullptr, 0, nullptr,
                         static_cast<uint32_t>(toAttachmentBarriers.size()), toAttachmentBarriers.data());

    VkRenderingAttachmentInfo posAttachment{};
    posAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    posAttachment.imageView = positionImageView;
    posAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    posAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    posAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    posAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 0.0f}};

    VkRenderingAttachmentInfo normalAttachment = posAttachment;
    normalAttachment.imageView = normalImageView;

    VkRenderingAttachmentInfo albedoAttachment = posAttachment;
    albedoAttachment.imageView = albedoImageView;

    std::array<VkRenderingAttachmentInfo, 3> colorAttachments = { posAttachment, normalAttachment, albedoAttachment };

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = depthImageView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil = { 1.0f, 0 };

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = { 0, 0 };
    renderingInfo.renderArea.extent = swapchain->swapChainExtent;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
    renderingInfo.pColorAttachments = colorAttachments.data();
    renderingInfo.pDepthAttachment = &depthAttachment;

    context->vkCmdBeginRenderingKHR(commandBuffer, &renderingInfo);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkViewport viewport{};
    viewport.width = static_cast<float>(swapchain->swapChainExtent.width);
    viewport.height = static_cast<float>(swapchain->swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent = swapchain->swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &uboDescriptorSet, 0, nullptr);

    glm::mat4 model0 = this->modelMatrices->at(0);
    for(const auto& mesh : model->subMeshes)
    {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &materialDescriptorSets[mesh.materialIndex], 0, nullptr);
        VkBuffer vertexBuffers[] = { mesh.vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model0);
        vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, 0, 0, 0);
    }

    context->vkCmdEndRenderingKHR(commandBuffer);

    for (auto &barrier : toAttachmentBarriers)
    {
        barrier.oldLayout = barrier.newLayout;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = barrier.subresourceRange.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT
                                    ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
                                    : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr,
                         static_cast<uint32_t>(toAttachmentBarriers.size()), toAttachmentBarriers.data());
                         
    firstFrame = false;
}

void GBufferPipeline::cleanup()
{
    vkDestroySampler(context->device, gbufferSampler, nullptr);

    vkDestroyImageView(context->device, positionImageView, nullptr);
    vkDestroyImage(context->device, positionImage, nullptr);
    vkFreeMemory(context->device, positionImageMemory, nullptr);

    vkDestroyImageView(context->device, normalImageView, nullptr);
    vkDestroyImage(context->device, normalImage, nullptr);
    vkFreeMemory(context->device, normalImageMemory, nullptr);

    vkDestroyImageView(context->device, albedoImageView, nullptr);
    vkDestroyImage(context->device, albedoImage, nullptr);
    vkFreeMemory(context->device, albedoImageMemory, nullptr);

    vkDestroyImageView(context->device, depthImageView, nullptr);
    vkDestroyImage(context->device, depthImage, nullptr);
    vkFreeMemory(context->device, depthImageMemory, nullptr);

    vkDestroyBuffer(context->device, uboBuffer, nullptr);
    vkFreeMemory(context->device, uboMemory, nullptr);

    vkDestroyDescriptorPool(context->device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(context->device, uboSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(context->device, materialSetLayout, nullptr);

    vkDestroyPipeline(context->device, pipeline, nullptr);
    vkDestroyPipelineLayout(context->device, pipelineLayout, nullptr);
}