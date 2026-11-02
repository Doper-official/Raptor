#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

const int MAX_LIGHTS = 10;

struct PointLight {
  vec4 position;
  vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  mat4 inverseView;

  vec4 ambientLightColor;

  vec4 directionalLightDirection;
  vec4 directionalLightColor;

  PointLight pointLights[MAX_LIGHTS];
  int numLights;
}
ubo;

// G-buffer images
layout(set = 1, binding = 0) uniform sampler2D albedoSampler;
layout(set = 1, binding = 1) uniform sampler2D normalSampler;
layout(set = 1, binding = 2) uniform sampler2D depthSampler;

void main() {
  vec2 uv = fragUV;

  vec3 albedo = texture(albedoSampler, uv).rgb;
  vec3 normal = normalize(texture(normalSampler, uv).xyz);
  float depth = texture(depthSampler, uv).r;

  // Don't light pixels containing no geometry.
  if (depth >= 0.999999) {
    outColor = vec4(0.0, 0.0, 0.0, 1.0);
    return;
  }

  // Reconstruct world position from depth.
  vec4 clipPosition = vec4(uv.x * 2.0 - 1.0, uv.y * 2.0 - 1.0, depth, 1.0);

  vec4 viewPosition = inverse(ubo.projection) * clipPosition;
  viewPosition /= viewPosition.w;

  vec4 worldPosition = ubo.inverseView * viewPosition;

  vec3 worldPos = worldPosition.xyz;

  // Camera position comes from the inverse view matrix.
  vec3 viewDir = normalize(ubo.inverseView[3].xyz - worldPos);

  // Ambient illumination.
  vec3 lighting = ubo.ambientLightColor.rgb * ubo.ambientLightColor.w * albedo;

  // ---------------------------------------------------------
  // Directional light
  // ---------------------------------------------------------

  // Directional light direction represents the direction
  // the rays travel, so negate it to get the direction
  // from the surface toward the light.
  vec3 lightDir = normalize(-ubo.directionalLightDirection.xyz);

  vec3 directionalColor =
      ubo.directionalLightColor.rgb * ubo.directionalLightColor.w;

  float diffuseFactor = max(dot(normal, lightDir), 0.0);

  vec3 diffuseLight = diffuseFactor * albedo * directionalColor;

  // Blinn-Phong specular highlight.
  float specularFactor = 0.0;

  if (diffuseFactor > 0.0) {
    vec3 halfwayDir = normalize(lightDir + viewDir);

    specularFactor = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
  }

  vec3 specularLight = specularFactor * directionalColor;

  lighting += diffuseLight + specularLight;

  // ---------------------------------------------------------
  // Point lights
  // ---------------------------------------------------------

  // Kept for future use.
  // With numLights = 0, this loop does nothing.
  int lightCount = clamp(ubo.numLights, 0, MAX_LIGHTS);

  for (int i = 0; i < lightCount; ++i) {
    PointLight light = ubo.pointLights[i];

    vec3 toLight = light.position.xyz - worldPos;

    float distanceToLight = length(toLight);

    if (distanceToLight <= 0.0001) {
      continue;
    }

    vec3 pointLightDir = toLight / distanceToLight;

    float pointDiffuseFactor = max(dot(normal, pointLightDir), 0.0);

    float pointSpecularFactor = 0.0;

    if (pointDiffuseFactor > 0.0) {
      vec3 halfwayDir = normalize(pointLightDir + viewDir);

      pointSpecularFactor = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    }

    // Simple distance attenuation.
    float attenuation = 1.0 / max(distanceToLight * distanceToLight, 0.01);

    vec3 pointLightColor = light.color.rgb * light.color.w * attenuation;

    vec3 pointDiffuseLight = pointDiffuseFactor * albedo * pointLightColor;

    vec3 pointSpecularLight = pointSpecularFactor * pointLightColor;

    lighting += pointDiffuseLight + pointSpecularLight;
  }

  outColor = vec4(lighting, 1.0);
}