#version 460 
#extension GL_EXT_ray_tracing : require

struct HitPayload {
    vec3 color;
    vec3 hitPos;
    vec3 hitNormal;
    vec3 albedo;
    int hit;
};

layout(location = 0) rayPayloadInEXT HitPayload payload;

void main()
{
    payload.color = vec3(0.0, 0.0, 0.0);
    payload.hit = 0;
}