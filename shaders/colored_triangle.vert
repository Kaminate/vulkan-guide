#version 450

#extension GL_KHR_vulkan_glsl: enable

layout( location = 0 ) out vec3 outColor;

void main()
{
  const vec3 positions[] =
  {
    { 1, 1, 0 },
    { -1, 1, 0 },
    { 0, -1, 0 },
  };
  const vec3 colors[] =
  {
    { 1, 0, 0 },
    { 0, 1, 0 },
    { 0, 0, 1 },
  };
  gl_Position = vec4( positions[ gl_VertexIndex ], 1 );
  outColor = colors[ gl_VertexIndex ];
}

