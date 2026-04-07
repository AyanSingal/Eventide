#include "CommandManager.h"
void CommandManager::init(VulkanContext& context){
    this->context = &context;
    createCommandPools();
    createCommandBuffers();
}
VkCommandBuffer CommandManager::beginSingleTimeCommands(VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(context->device, &allocInfo, &commandBuffer);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}
void CommandManager::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue queueFamily)
{
    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(queueFamily, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queueFamily);
    vkFreeCommandBuffers(context->device, commandPool, 1, &commandBuffer);
}
void CommandManager::createCommandPools()
{
    QueueFamilyIndices queueFamilyIndices = context->findQueueFamilies(context->physicalDevice);
    VkCommandPoolCreateInfo graphicsPoolInfo{};
    graphicsPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    graphicsPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    graphicsPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    VkCommandPoolCreateInfo transferPoolInfo{};
    transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    transferPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    transferPoolInfo.queueFamilyIndex = queueFamilyIndices.transferFamily.value();
    if (vkCreateCommandPool(context->device, &graphicsPoolInfo, nullptr, &graphicsCommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics command pool!");
    }
    if (vkCreateCommandPool(context->device, &transferPoolInfo, nullptr, &transferCommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create transfer command pool!");
    }
}
void CommandManager::createCommandBuffers()
{
    // Graphics command buffers setup
    graphicsCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo graphicsBufferAllocInfo{};
    graphicsBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    graphicsBufferAllocInfo.commandPool = graphicsCommandPool;
    graphicsBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    graphicsBufferAllocInfo.commandBufferCount = (uint32_t)graphicsCommandBuffers.size();
    if (vkAllocateCommandBuffers(context->device, &graphicsBufferAllocInfo, graphicsCommandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate graphics command buffers!");
    }
}
void CommandManager::cleanup(){
    vkDestroyCommandPool(context->device, graphicsCommandPool, nullptr);
    vkDestroyCommandPool(context->device, transferCommandPool, nullptr);
}