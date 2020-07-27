#version 450
//#extension GL_ARB_separate_shader_objects : enable

//out gl_PerVertex {
//  vec4 gl_Position;
//};

layout(binding = 0) uniform UniformBufferObject{
  mat4 model;
  mat4 view;
  mat4 proj;
}ubo;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 texCoord;

layout(location = 1) out vec2 fragTexCoord;

void main() {
  gl_Position = ubo.proj * ubo.view * ubo.model * vec4(position,1.f);
  fragTexCoord = texCoord;
}
