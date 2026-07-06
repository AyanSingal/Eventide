#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require

layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(location = 1) rayPayloadEXT bool shadowed;
hitAttributeEXT vec2 baryCoords;

struct Vertex {
    vec3 pos;
    vec3 color;
    vec2 texCoord;
    vec3 normal;
};

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(scalar, set = 0, binding = 3) buffer VertexBuffers { Vertex v[]; } vertices[];
layout(scalar, set = 0, binding = 4) buffer IndexBuffers { uint i[]; } indices[];
layout(set = 0, binding = 5) uniform sampler2D textures[];

void main()
{
    int meshId = gl_InstanceCustomIndexEXT;

    uint i0 = indices[nonuniformEXT(meshId)].i[gl_PrimitiveID * 3 + 0];
    uint i1 = indices[nonuniformEXT(meshId)].i[gl_PrimitiveID * 3 + 1];
    uint i2 = indices[nonuniformEXT(meshId)].i[gl_PrimitiveID * 3 + 2];

    Vertex v0 = vertices[nonuniformEXT(meshId)].v[i0];
    Vertex v1 = vertices[nonuniformEXT(meshId)].v[i1];
    Vertex v2 = vertices[nonuniformEXT(meshId)].v[i2];

    float w = 1.0 - baryCoords.x - baryCoords.y;

    // Interpolate UVs
    vec2 texCoord = w * v0.texCoord + baryCoords.x * v1.texCoord + baryCoords.y * v2.texCoord;

    // Interpolate normals (and transform to world space via the object's transform)
    vec3 normal = normalize(w * v0.normal + baryCoords.x * v1.normal + baryCoords.y * v2.normal);
    normal = normalize(vec3(gl_ObjectToWorldEXT * vec4(normal, 0.0)));

    // Sample base color
    vec3 baseColor = texture(textures[nonuniformEXT(meshId)], texCoord).rgb;

    // Directional light coming from above and slightly to the side
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diffuse = max(dot(normal, lightDir), 0.0);

    vec3 hitPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

    if (diffuse > 0.0){
        shadowed = true;

        traceRayEXT(topLevelAS,
            gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT,
            0xFF,
            0, 0,   // sbtRecordOffset, sbtRecordStride
            1,      // miss shader index (1 = our new shadow miss shader)
            hitPos + normal * 0.001,  // origin (offset slightly to avoid self-intersection)
            0.001,  // tmin
            lightDir,
            10000.0, // tmax
            1);     // payload location

        if (shadowed) {
            diffuse = 0.0;
        }
    }

    // A bit of ambient so unlit surfaces aren't pitch black
    float ambient = 0.15;

    hitValue = baseColor * (diffuse + ambient);
}