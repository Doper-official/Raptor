#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormalWorld;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;

void main() {
  // G-buffer attachment 0: surface color
  outAlbedo = vec4(fragColor, 1.0);

  // G-buffer attachment 1: world-space surface normal
  outNormal = vec4(normalize(fragNormalWorld), 1.0);
}