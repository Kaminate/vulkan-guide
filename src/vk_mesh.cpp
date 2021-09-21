#include <vk_mesh.h>

VertexInputDescription Vertex::get_vertex_description()
{
  VertexInputDescription description;
  description.bindings = {
  []() {
    VkVertexInputBindingDescription mainBinding = {};
    mainBinding.stride = sizeof( Vertex );
    mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return mainBinding;
  }( ) };
  description.attributes = {
    []() {
        VkVertexInputAttributeDescription attribute = {};
        attribute.location = 0;
        attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute.offset = offsetof( Vertex, position );
        return attribute;
    }( ),
    []() {
        VkVertexInputAttributeDescription attribute = {};
        attribute.location = 1;
        attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute.offset = offsetof( Vertex, normal );
        return attribute;
    }( ),
    []() {
        VkVertexInputAttributeDescription attribute = {};
        attribute.location = 2;
        attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute.offset = offsetof( Vertex, color );
        return attribute;
    }( )
  };
  return description;
}



