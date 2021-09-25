#include <vk_mesh.h>
#include <tiny_obj_loader.h>
#include <iostream>

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



bool Mesh::load_from_obj( const char* path )
{
  tinyobj::attrib_t attrib;
  std::vector< tinyobj::shape_t > shapes;
  std::vector< tinyobj::material_t > materials;
  std::string warn;
  std::string err;
  tinyobj::LoadObj( &attrib, &shapes, &materials, &warn, &err, path, nullptr );
  if( !warn.empty() )
    std::cout << warn << std::endl;
  if( !err.empty() )
    std::cout << err << std::endl;
  for( size_t s = 0; s < shapes.size(); ++s )
  {
    int index_offset = 0;
    tinyobj::shape_t* shape = &shapes[ s ];
    const int face_count = ( int )shape->mesh.num_face_vertices.size();
    for( int f = 0; f < face_count; ++f )
    {
      int fv = 3;
      for( size_t v = 0; v < fv; ++v )
      {
        tinyobj::index_t idx = shape->mesh.indices[ index_offset + v ];
        Vertex new_vert;
        new_vert.position.x = attrib.vertices[ 3 * idx.vertex_index + 0 ];
        new_vert.position.y = attrib.vertices[ 3 * idx.vertex_index + 1 ];
        new_vert.position.z = attrib.vertices[ 3 * idx.vertex_index + 2 ];
        new_vert.normal.x = attrib.normals[ 3 * idx.normal_index + 0 ];
        new_vert.normal.y = attrib.normals[ 3 * idx.normal_index + 1 ];
        new_vert.normal.z = attrib.normals[ 3 * idx.normal_index + 2 ];
        new_vert.color = new_vert.normal;
        _verticies.push_back( new_vert );
      }
      index_offset += ( int )shape->mesh.num_face_vertices[ f ];
    }
  }
  return err.empty();
}
