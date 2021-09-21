#version 450

#extension GL_KHR_vulkan_glsl: enable

void main()
{
  const vec3 positions[] =
  {
    { 1, 1, 0 },
    { -1, 1, 0 },
    { 0, -1, 0 },
  };
  gl_Position = vec4( positions[ gl_VertexIndex ], 1 );
}

