#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

const int MAX_LIGHTS = 10;

struct PointLight {
  vec4 position;
  vec4 color;
};

layout(std140, set = 0, binding = 0) uniform GlobalUbo {
  mat4 projection;
  mat4 view;
  mat4 inverseView;
  mat4 directionalLightViewProjection;

  vec4 ambientLightColor;
  vec4 directionalLightDirection;
  vec4 directionalLightColor;

  PointLight pointLights[MAX_LIGHTS];
  int numLights;
}
ubo;

layout(set = 1, binding = 0) uniform sampler2D albedoSampler;
layout(set = 1, binding = 1) uniform sampler2D normalSampler;
layout(set = 1, binding = 2) uniform sampler2D depthSampler;
layout(set = 1, binding = 3) uniform sampler2D directionalShadowSampler;

// Shadows go brrrr
float calculateDirectionalShadow(vec3 worldPos, vec3 normal, vec3 lightDir) {
  vec4 lightClip = ubo.directionalLightViewProjection * vec4(worldPos, 1.0);

  if (lightClip.w <= 0.0) {
    return 0.0;
  }

  vec3 lightNdc = lightClip.xyz / lightClip.w;
  vec2 shadowUV = lightNdc.xy * 0.5 + 0.5;

  // Outside the light camera = no shadow.
  if (lightNdc.z < 0.0 || lightNdc.z > 1.0 || shadowUV.x < 0.0 ||
      shadowUV.x > 1.0 || shadowUV.y < 0.0 || shadowUV.y > 1.0) {
    return 0.0;
  }

  float nDotL = max(dot(normal, lightDir), 0.0);

  float bias = max(0.0012 * (1.0 - nDotL), 0.00025);

  vec2 texelSize = 1.0 / vec2(textureSize(directionalShadowSampler, 0));

  float shadow = 0.0;

  // 3x3 PCF, but with interpolated comparisons.
  for (int x = -1; x <= 1; ++x) {
    for (int y = -1; y <= 1; ++y) {

      vec2 sampleUV = shadowUV + vec2(x, y) * texelSize;

      // Don't sample outside the shadow map.
      if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 ||
          sampleUV.y > 1.0) {
        continue;
      }

      float storedDepth = texture(directionalShadowSampler, sampleUV).r;

      float depthDifference = lightNdc.z - bias - storedDepth;

      // Interpolate the shadow boundary per fragment.
      float gradientWidth = max(fwidth(depthDifference) * 1.5, 0.00035);

      float shadowSample =
          smoothstep(-gradientWidth, gradientWidth, depthDifference);

      shadow += shadowSample;
    }
  }

  return shadow / 9.0;
}

void main() {
  vec2 uv = fragUV;

  float depth = texture(depthSampler, uv).r;

  if (depth >= 0.999999) {
    outColor = vec4(0.0, 0.0, 0.0, 1.0);
    return;
  }

  vec3 albedo = texture(albedoSampler, uv).rgb;

  vec3 normal = normalize(texture(normalSampler, uv).xyz);

  // Reconstruct world position.
  vec4 clipPosition = vec4(uv * 2.0 - 1.0, depth, 1.0);

  vec4 viewPosition = inverse(ubo.projection) * clipPosition;

  viewPosition /= viewPosition.w;

  vec3 worldPos = (ubo.inverseView * viewPosition).xyz;

  vec3 viewDir = normalize(ubo.inverseView[3].xyz - worldPos);

  // Ambient illumination.
  vec3 lighting = ubo.ambientLightColor.rgb * ubo.ambientLightColor.w * albedo;

  // The UBO stores ray direction.
  // Flip it to point toward the sun.
  vec3 lightDir = normalize(-ubo.directionalLightDirection.xyz);

  vec3 directionalColor =
      ubo.directionalLightColor.rgb * ubo.directionalLightColor.w;

  // Directional diffuse.
  float diffuseFactor = max(dot(normal, lightDir), 0.0);

  float shadow = diffuseFactor > 0.0
                     ? calculateDirectionalShadow(worldPos, normal, lightDir)
                     : 0.0;

  float visibility = 1.0 - shadow;

  vec3 diffuseLight = diffuseFactor * albedo * directionalColor * visibility;

  // Directional specular.
  float specularFactor = 0.0;

  if (diffuseFactor > 0.0) {
    vec3 halfwayDir = normalize(lightDir + viewDir);

    specularFactor = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
  }

  vec3 specularLight = specularFactor * directionalColor * visibility;

  lighting += diffuseLight + specularLight;

  // Point lights are still supported.
  // They don't have shadow maps yet. Cube maps later :D
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

    float attenuation = 1.0 / max(distanceToLight * distanceToLight, 0.01);

    vec3 pointLightColor = light.color.rgb * light.color.w * attenuation;

    lighting += pointDiffuseFactor * albedo * pointLightColor;

    lighting += pointSpecularFactor * pointLightColor;
  }

  outColor = vec4(lighting, 1.0);
}