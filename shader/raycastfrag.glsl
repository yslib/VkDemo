#version 450

layout(binding = 4)uniform RaycastParams{
  uniform float step;
  uniform vec2 resolution;
}param;

layout(binding = 0) uniform sampler2D entryPosTex;
layout(binding = 1) uniform sampler2D exitPosTex;
layout(binding = 2) uniform sampler3D volumeTex;
layout(binding = 3) uniform sampler1D tfTex;

layout(location = 0) out vec4 fragColor;

void main()
{
  vec2 uv = vec2(gl_FragCoord)/param.resolution;
  vec3 start = texture(entryPosTex,uv).xyz;
  vec3 end = texture(entryPosTex,uv).xyz;
  vec3 dir = normalize(end-start);
  int count = int(length(end - start) / param.step);
  vec4 color =vec4(0.0);
  for(int i = 0 ; i < count ; i++){
    vec3 samplePoint = start + dir * i *param.step;
    vec4 sampledColor = texture(tfTex,texture(volumeTex,samplePoint).r);
		color = color + sampledColor * vec4( sampledColor.aaa, 1.0 ) * ( 1.0 - color.a );
    if(color.a > 0.99)
      break;
  }
  fragColor =  color;
}

