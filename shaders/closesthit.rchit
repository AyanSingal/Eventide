#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec2 baryCoords;

struct Vertex {
    vec3 pos;
    vec3 color;
    vec2 texCoord;
};

layout(scalar, set = 0, binding = 3) buffer VertexBuffers { Vertex v[]; } vertices[];
layout(scalar, set = 0, binding = 4) buffer IndexBuffers { uint i[]; } indices[];
layout(set = 0, binding = 5) uniform sampler2D textures[];

void main()
{
    int meshId = gl_InstanceCustomIndexEXT;

    uint i0 = indices[meshId].i[gl_PrimitiveID * 3 + 0];
    uint i1 = indices[meshId].i[gl_PrimitiveID * 3 + 1];
    uint i2 = indices[meshId].i[gl_PrimitiveID * 3 + 2];

    vec2 uv0 = vertices[meshId].v[i0].texCoord;
    vec2 uv1 = vertices[meshId].v[i1].texCoord;
    vec2 uv2 = vertices[meshId].v[i2].texCoord;

    float w = 1.0 - baryCoords.x - baryCoords.y;
    vec2 texCoord = w * uv0 + baryCoords.x * uv1 + baryCoords.y * uv2;

    hitValue = texture(textures[nonuniformEXT(meshId)], texCoord).rgb;
}