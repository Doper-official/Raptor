#version 450

layout(location = 0) in vec3 position;

layout(push_constant) uniform ShadowPush {
    mat4 lightViewProjection;
    mat4 modelMatrix;
} push;

void main() {
    gl_Position = push.lightViewProjection * push.modelMatrix * vec4(position, 1.0);
}
