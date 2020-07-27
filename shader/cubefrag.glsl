#version 450
layout(location = 0)in vec3 pos;
layout(location = 0)out vec3 rasterPos;
void main()
{
  rasterPos = pos;
}
