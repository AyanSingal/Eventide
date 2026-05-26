#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadEXT vec3 hitValue;

hitAttributeEXT vec2 baryCoords;

void main()
{
    vec3 barycentrics = vec3(1.0 - baryCoords.x - baryCoords.y, baryCoords.x, baryCoords.y);
    hitValue = barycentrics;
}