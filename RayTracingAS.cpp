#include "RayTracingAS.h"

void RayTracingAS::init(VulkanContext &context, ResourceManager &resourceManager, CommandManager &commandManager, VulkanModel &model, const std::vector<glm::mat4>& modelMatrices)
{
    this->context = &context;
    this->resourceManager = &resourceManager;
    this->commandManager = &commandManager;
    this->modelMatrices = modelMatrices;
    buildBLAS(model);
    buildTLAS();
}

void RayTracingAS::buildBLAS(VulkanModel &model)
{
    BLASes.resize(model.subMeshes.size());

    std::cout << "Building BLASes for " << model.subMeshes.size() << " sub-meshes" << std::endl;

    for(size_t i = 0; i < model.subMeshes.size(); i++)
    {
        const auto& mesh = model.subMeshes[i];

        // Grab the addresses of vertex and index buffers
        VkBufferDeviceAddressInfo vertexAddrInfo{};
        vertexAddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        vertexAddrInfo.buffer = mesh.vertexBuffer;
        VkDeviceAddress vertexAddress = context->vkGetBufferDeviceAddressKHR(context->device, &vertexAddrInfo);

        VkBufferDeviceAddressInfo  indexAddrInfo{};
        indexAddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        indexAddrInfo.buffer = mesh.indexBuffer;
        VkDeviceAddress indexAddress = context->vkGetBufferDeviceAddressKHR(context->device, &indexAddrInfo);


        // Describe triangle geometry
        VkAccelerationStructureGeometryTrianglesDataKHR triangles{};
        triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        triangles.vertexData.deviceAddress = vertexAddress;
        triangles.vertexStride = sizeof(Vertex);
        triangles.maxVertex = mesh.indexCount;
        triangles.indexType = VK_INDEX_TYPE_UINT32;
        triangles.indexData.deviceAddress = indexAddress;

        VkAccelerationStructureGeometryKHR geometry{};
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.geometry.triangles = triangles;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometry;

        // Query the size requirement
        uint32_t primitiveCount = mesh.indexCount / 3;
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
        sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        context->vkGetAccelerationStructureBuildSizesKHR(
            context->device,
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &buildInfo,
            &primitiveCount,
            &sizeInfo
        );

        // Create the buffer and acceleration structure
        resourceManager->createBuffer(
            sizeInfo.accelerationStructureSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            BLASes[i].blasBuffer, BLASes[i].blasBufferMemory);
        
        VkAccelerationStructureCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        createInfo.buffer = BLASes[i].blasBuffer;
        createInfo.size = sizeInfo.accelerationStructureSize;
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        context->vkCreateAccelerationStructureKHR(context->device, &createInfo, nullptr, &BLASes[i].blasHandle);

        // Create a scratch buffer for the build
        VkBuffer scratchBuffer;
        VkDeviceMemory scratchMemory;
        resourceManager->createBuffer(
            sizeInfo.buildScratchSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            scratchBuffer, scratchMemory);
        
        VkBufferDeviceAddressInfo scratchAddrInfo{};
        scratchAddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        scratchAddrInfo.buffer = scratchBuffer;
        VkDeviceAddress scratchAddress = context->vkGetBufferDeviceAddressKHR(context->device, &scratchAddrInfo);

        // Record and submit build command
        buildInfo.dstAccelerationStructure = BLASes[i].blasHandle;
        buildInfo.scratchData.deviceAddress = scratchAddress;

        VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
        rangeInfo.primitiveCount = primitiveCount;
        rangeInfo.primitiveOffset = 0;
        rangeInfo.firstVertex = 0;
        rangeInfo.transformOffset = 0;
        const VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

        VkCommandBuffer cmd = commandManager->beginSingleTimeCommands(commandManager->graphicsCommandPool);
        context->vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, &pRangeInfo);
        commandManager->endSingleTimeCommands(cmd, commandManager->graphicsCommandPool, context->graphicsQueue);

        vkDestroyBuffer(context->device, scratchBuffer, nullptr);
        vkFreeMemory(context->device, scratchMemory, nullptr);
    }

    std::cout << "Built " << BLASes.size() << " BLASes" << std::endl;
}

void RayTracingAS::buildTLAS()
{
    std::vector<VkAccelerationStructureInstanceKHR> instances;

    for(size_t i = 0; i < BLASes.size(); i++)
    {
        //Grab the device address of this BLAS
        VkAccelerationStructureDeviceAddressInfoKHR addrInfo{};
        addrInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        addrInfo.accelerationStructure = BLASes[i].blasHandle;
        VkDeviceAddress blasAddress = context->vkGetAccelerationStructureDeviceAddressKHR(context->device, &addrInfo);

        glm::mat4 model = modelMatrices[0];
        VkTransformMatrixKHR transform{};
        for (int row = 0; row < 3; row++)
        {
            for (int col = 0; col < 4; col++)
            {
                transform.matrix[row][col] = model[col][row]; //transpose the matrix
            }
        }

        VkAccelerationStructureInstanceKHR instance{};
        instance.transform = transform;
        instance.instanceCustomIndex = i;
        instance.mask = 0xFF;
        instance.instanceShaderBindingTableRecordOffset = 0;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.accelerationStructureReference = blasAddress;

        instances.push_back(instance);
    }

    VkDeviceSize instanceSize = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();

    VkBuffer instanceStaging;
    VkDeviceMemory instanceStagingMemory;

    resourceManager->createBuffer(instanceSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        instanceStaging, instanceStagingMemory);

    void* data;
    vkMapMemory(context->device, instanceStagingMemory, 0, instanceSize, 0, &data);
    memcpy(data, instances.data(), instanceSize);
    vkUnmapMemory(context->device, instanceStagingMemory);

    resourceManager->createBuffer(instanceSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        tlas.instanceBuffer, tlas.instanceBufferMemory);

    resourceManager->copyBuffer(instanceStaging, tlas.instanceBuffer, instanceSize);

    vkDestroyBuffer(context->device, instanceStaging, nullptr);
    vkFreeMemory(context->device, instanceStagingMemory, nullptr);

    // Get instance buffer device address
    VkBufferDeviceAddressInfo instanceAddrInfo{};
    instanceAddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    instanceAddrInfo.buffer = tlas.instanceBuffer;
    VkDeviceAddress instanceAddress = context->vkGetBufferDeviceAddressKHR(context->device, &instanceAddrInfo);

    // Describe the TLAS geometry (instances)
    VkAccelerationStructureGeometryInstancesDataKHR instancesData{};
    instancesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    instancesData.data.deviceAddress = instanceAddress;

    VkAccelerationStructureGeometryKHR tlasGeometry{};
    tlasGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    tlasGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlasGeometry.geometry.instances = instancesData;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &tlasGeometry;

    // Query size
    uint32_t instanceCount = (uint32_t) instances.size();
    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
    sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    context->vkGetAccelerationStructureBuildSizesKHR(
        context->device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &buildInfo,
        &instanceCount,
        &sizeInfo);

    // Create TLAS buffer and structure
    resourceManager->createBuffer(
        sizeInfo.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        tlas.tlasBuffer, tlas.tlasBufferMemory);
    
    VkAccelerationStructureCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    createInfo.buffer = tlas.tlasBuffer;
    createInfo.size = sizeInfo.accelerationStructureSize;
    createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    context->vkCreateAccelerationStructureKHR(context->device, &createInfo, nullptr, &tlas.tlasHandle);

    //Scratch buffer
    VkBuffer scratchBuffer;
    VkDeviceMemory scratchMemory;
    resourceManager->createBuffer(
        sizeInfo.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        scratchBuffer, scratchMemory);
    
    VkBufferDeviceAddressInfo scratchAddrInfo{};
    scratchAddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    scratchAddrInfo.buffer = scratchBuffer;
    VkDeviceAddress scratchAddress = context->vkGetBufferDeviceAddressKHR(context->device, &scratchAddrInfo);

    //Build
    buildInfo.dstAccelerationStructure = tlas.tlasHandle;
    buildInfo.scratchData.deviceAddress = scratchAddress;

    VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
    rangeInfo.primitiveCount = instances.size();
    rangeInfo.primitiveOffset = 0;
    rangeInfo.firstVertex = 0;
    rangeInfo.transformOffset = 0;
    const VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

    VkCommandBuffer cmd = commandManager->beginSingleTimeCommands(commandManager->graphicsCommandPool);
    context->vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, &pRangeInfo);
    commandManager->endSingleTimeCommands(cmd, commandManager->graphicsCommandPool, context->graphicsQueue);

    vkDestroyBuffer(context->device, scratchBuffer, nullptr);
    vkFreeMemory(context->device, scratchMemory, nullptr);

    std::cout << "Built TLAS with " << instanceCount << " instances" << std::endl;

}


void RayTracingAS::cleanup()
{
    context->vkDestroyAccelerationStructureKHR(context->device, tlas.tlasHandle, nullptr);
    vkDestroyBuffer(context->device, tlas.instanceBuffer, nullptr);
    vkFreeMemory(context->device, tlas.instanceBufferMemory, nullptr);
    vkDestroyBuffer(context->device, tlas.tlasBuffer, nullptr);
    vkFreeMemory(context->device, tlas.tlasBufferMemory, nullptr);
    

    for (auto &blas : BLASes)
    {
        context->vkDestroyAccelerationStructureKHR(context->device, blas.blasHandle, nullptr);
        vkDestroyBuffer(context->device, blas.blasBuffer, nullptr);
        vkFreeMemory(context->device, blas.blasBufferMemory, nullptr);
    }
}