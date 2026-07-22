#version 460

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragWorldNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedo;

void main() {
    outPosition = vec4(fragWorldPos, 1.0);
    outNormal = vec4(normalize(fragWorldNormal), 0.0);
    outAlbedo = vec4(texture(texSampler, fragTexCoord).rgb, 1.0);
}