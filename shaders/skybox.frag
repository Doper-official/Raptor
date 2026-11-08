#version 450

layout(location = 0) in vec2 inNdc;
layout(location = 0) out vec4 outColor;

layout(std140, set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  mat4 inverseView;
  mat4 directionalLightViewProjection;
  vec4 ambientLightColor;
  vec4 directionalLightDirection;
}
ubo;

layout(set = 1, binding = 0) uniform samplerCube environmentMap;

layout(push_constant) uniform SkyboxPushConstants {
  mat4 inverseProjection;
  vec4 time;
}
pc;

void main() {
  vec4 viewPosition = pc.inverseProjection * vec4(inNdc, 1.0, 1.0);

  vec3 viewDirection = normalize(viewPosition.xyz / viewPosition.w);

  vec3 worldDirection = normalize(mat3(ubo.inverseView) * viewDirection);

  vec3 color = texture(environmentMap, worldDirection).rgb;

  outColor = vec4(color, 1.0);
}