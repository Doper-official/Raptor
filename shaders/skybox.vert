#version 450

layout(location = 0) out vec2 outNdc;

void main() {
  // Oversized fullscreen triangle. No vertex buffer is needed.
  vec2 position =
      vec2(float((gl_VertexIndex << 1) & 2), float(gl_VertexIndex & 2));

  position = position * 2.0 - 1.0;

  outNdc = position;
  gl_Position = vec4(position, 1.0, 1.0);
}
