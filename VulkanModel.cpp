#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "VulkanModel.h"
#include <tiny_obj_loader.h>

void VulkanModel::init(VulkanContext &context, ResourceManager &resourceManager, const std::string &modelPath)
{
    this->context = &context;
    this->resourceManager = &resourceManager;
    load(modelPath);
    createVertexBuffer();
    createIndexBuffer();
}

void VulkanModel::load(const std::string &modelPath)
{
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, modelPath);

    if (!warn.empty())
    {
        std::cout << "glTF warning: " << warn << std::endl;
    }
    if (!err.empty())
    {
        std::cout << "glTF warning: " << err << std::endl;
    }
    if (!ret)
    {
        throw std::runtime_error("Failed to load glTF: " + modelPath);
    }

    for (const auto &mesh : gltfModel.meshes)
    {
        for (const auto &primitive : mesh.primitives)
        {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;

            // Extract vertex positions
            const tinygltf::Accessor &posAccessor = gltfModel.accessors[primitive.attributes.find("POSITION")->second];
            const tinygltf::BufferView &posView = gltfModel.bufferViews[posAccessor.bufferView];
            const tinygltf::Buffer &posBuffer = gltfModel.buffers[posView.buffer];
            const float *positions = reinterpret_cast<const float *>(&posBuffer.data[posView.byteOffset + posAccessor.byteOffset]);

            // Extract texture coordinates
            const float *texcoords = nullptr;
            if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
            {
                const tinygltf::Accessor &texAccessor = gltfModel.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                const tinygltf::BufferView &texView = gltfModel.bufferViews[texAccessor.bufferView];
                const tinygltf::Buffer &texBuffer = gltfModel.buffers[texView.buffer];
                texcoords = reinterpret_cast<const float *>(&texBuffer.data[texView.byteOffset + texAccessor.byteOffset]);
            }

            // Populate the vertices array for this primitive
            for (size_t i = 0; i < posAccessor.count; i++)
            {
                Vertex vertex{};
                vertex.pos = {
                    positions[i * 3 + 0],
                    positions[i * 3 + 1],
                    positions[i * 3 + 2]};
                vertex.color = {1.0f, 1.0f, 1.0f};
                if (texcoords)
                {
                    vertex.texCoord = {
                        texcoords[i * 2 + 0],
                        texcoords[i * 2 + 1]};
                }
                else
                {
                    vertex.texCoord = {0.0f, 0.0f};
                }
                vertices.push_back(vertex);
            }

            // Extract indices
            const tinygltf::Accessor &indexAccessor = gltfModel.accessors[primitive.indices];
            const tinygltf::BufferView &indexView = gltfModel.bufferViews[indexAccessor.bufferView];
            const tinygltf::Buffer &indexBuffer = gltfModel.buffers[indexView.buffer];
            const unsigned char *indexData = &indexBuffer.data[indexView.byteOffset + indexAccessor.byteOffset];

            // Populate indices array for this primitive
            for (size_t i = 0; i < indexAccessor.count; i++)
            {
                uint32_t index;
                if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                {
                    index = reinterpret_cast<const uint16_t *>(indexData)[i];
                }
                else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                {
                    index = reinterpret_cast<const uint32_t *>(indexData)[i];
                }
                else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                {
                    index = indexData[i];
                }
                else
                {
                    throw std::runtime_error("Unsupported index type");
                }
                indices.push_back(index);
            }

            // Start setting up the submesh
            SubMesh subMesh{};
            subMesh.indexCount = static_cast<uint32_t>(indices.size());
            subMesh.materialIndex = primitive.material;

            // Fill in the vertex data for the submesh
            VkDeviceSize vertexSize = sizeof(Vertex) * vertices.size();
            VkBuffer vertexStaging;
            VkDeviceMemory vertexStagingMem;
            resourceManager->createBuffer(vertexSize,
                                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                          vertexStaging, vertexStagingMem);
            void *data;
            vkMapMemory(context->device, vertexStagingMem, 0, vertexSize, 0, &data);
            memcpy(data, vertices.data(), vertexSize);
            vkUnmapMemory(context->device, vertexStagingMem);
            resourceManager->createBuffer(vertexSize,
                                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                          subMesh.vertexBuffer, subMesh.vertexBufferMemory);
            resourceManager->copyBuffer(vertexStaging, subMesh.vertexBuffer, vertexSize);
            vkDestroyBuffer(context->device, vertexStaging, nullptr);
            vkFreeMemory(context->device, vertexStagingMem, nullptr);

            // Fill in the index data for the submesh
            VkDeviceSize indexSize = sizeof(uint32_t) * indices.size();
            VkBuffer indexStaging;
            VkDeviceMemory indexStagingMem;
            resourceManager->createBuffer(indexSize,
                                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                          indexStaging, indexStagingMem);
            void *data;
            vkMapMemory(context->device, indexStagingMem, 0, indexSize, 0, &data);
            memcpy(data, indices.data(), indexSize);
            vkUnmapMemory(context->device, indexStagingMem);
            resourceManager->createBuffer(indexSize,
                                          VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                          subMesh.indexBuffer, subMesh.indexBufferMemory);
            resourceManager->copyBuffer(indexStaging, subMesh.indexBuffer, indexSize);
            vkDestroyBuffer(context->device, indexStaging, nullptr);
            vkFreeMemory(context->device, indexStagingMem, nullptr);
            subMeshes.push_back(subMesh);
        }
    }

    std::cout << "Loaded " << subMeshes.size() << " sub-meshes" << std::endl;

    // tinyobj::attrib_t attrib;
    // std::vector<tinyobj::shape_t> shapes;
    // std::vector<tinyobj::material_t> materials;
    // std::string warn, err;

    // if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.c_str()))
    // {
    //     throw std::runtime_error(err);
    // }

    // std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    // for (const auto &shape : shapes)
    // {
    //     for (const auto &index : shape.mesh.indices)
    //     {
    //         Vertex vertex{};

    //         vertex.pos = {
    //             attrib.vertices[3 * index.vertex_index + 0],
    //             attrib.vertices[3 * index.vertex_index + 1],
    //             attrib.vertices[3 * index.vertex_index + 2]};

    //         vertex.texCoord = {
    //             attrib.texcoords[2 * index.texcoord_index + 0],
    //             1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

    //         vertex.color = {1.0f, 1.0f, 1.0f};

    //         if (uniqueVertices.count(vertex) == 0)
    //         {
    //             uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
    //             vertices.push_back(vertex);
    //         }

    //         indices.push_back(uniqueVertices[vertex]);
    //     }
    // }
}

void VulkanModel::createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    resourceManager->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void *data;
    vkMapMemory(context->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(context->device, stagingBufferMemory);

    resourceManager->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

    resourceManager->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
    vkDestroyBuffer(context->device, stagingBuffer, nullptr);
    vkFreeMemory(context->device, stagingBufferMemory, nullptr);
}

void VulkanModel::createIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    resourceManager->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void *data;
    vkMapMemory(context->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(context->device, stagingBufferMemory);

    resourceManager->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    resourceManager->copyBuffer(stagingBuffer, indexBuffer, bufferSize);
    vkDestroyBuffer(context->device, stagingBuffer, nullptr);
    vkFreeMemory(context->device, stagingBufferMemory, nullptr);
}

void VulkanModel::cleanup()
{
    vkDestroyBuffer(context->device, indexBuffer, nullptr);
    vkFreeMemory(context->device, indexBufferMemory, nullptr);

    vkDestroyBuffer(context->device, vertexBuffer, nullptr);
    vkFreeMemory(context->device, vertexBufferMemory, nullptr);
}