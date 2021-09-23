#include "vk_engine.h"
#include "vk_pipeline.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_types.h>
#include <vk_initializers.h>
#include <iostream>
#include <array>
#include <fstream>
#include <glm/gtx/transform.hpp>

#include "VkBootstrap.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

static std::vector< char > file_to_bytes( const char* path )
{
  std::ifstream ifs( path, std::ios::ate | std::ios::binary );
  if( !ifs.is_open() )
    return {};
  const int fileSize = ( int )ifs.tellg();
  ifs.seekg( 0 );
  std::vector< char > buffer( fileSize );
  ifs.read( buffer.data(), fileSize );
  return buffer;
}

static int round_up_nearest_multiple( int val, int mult )
{
  return ( ( val + mult - 1 ) / mult ) * mult;
}


void VK_CHECK( VkResult err )
{
  if( err == VK_SUCCESS )
    return;
  std::cout << "Detected Vulkan Error: " << err << std::endl;
  abort();
}

void VulkanEngine::init()
{
  SDL_Init( SDL_INIT_VIDEO );

  SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN;

  _window = SDL_CreateWindow( "Vulkan Engine",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              _windowExtent.width,
                              _windowExtent.height,
                              window_flags );

  init_vulkan();
  init_swapchain();
  init_commands();
  init_default_renderpass();
  init_framebuffers();
  init_sync_structures();
  init_pipelines();
  load_meshes();

  _isInitialized = true;
}

void VulkanEngine::cleanup()
{
  if( _isInitialized )
  {
    // Destroying the pool destroys all its comand buffers
    vkDestroyCommandPool( _device, _commandPool, nullptr );

    vkDestroySwapchainKHR( _device, _swapchain, nullptr );

    vkDestroyRenderPass( _device, _renderPass, nullptr );

    for( auto framebuffer : _framebuffers )
      vkDestroyFramebuffer( _device, framebuffer, nullptr );

    // Only need to destroy the imageviews and not the images
    // because the images are destroyed with the swap chain
    for( auto view : _swapchainImageViews )
      vkDestroyImageView( _device, view, nullptr );

    vkDestroyDevice( _device, nullptr );
    vkDestroySurfaceKHR( _instance, _surface, nullptr );
    vkb::destroy_debug_utils_messenger( _instance, _debug_messenger );
    vkDestroyInstance( _instance, nullptr );
    SDL_DestroyWindow( _window );
  }
}

void VulkanEngine::draw()
{
  const uint64_t one_sec_in_ns = 1000000000;
  // wait for gpu to finish rendering last frame
  // 1s timeout
  //
  // fence is for cpu sync, semaphore for the gpu sync
  VK_CHECK( vkWaitForFences( _device, 1, &_renderFence, true, one_sec_in_ns ) );
  VK_CHECK( vkResetFences( _device, 1, &_renderFence ) );

  uint32_t iSwapchainImage;
  VK_CHECK( vkAcquireNextImageKHR( _device,
                                   _swapchain,
                                   one_sec_in_ns,
                                   _presentSemaphore,
                                   nullptr, // choosing not to signal any fence here
                                   &iSwapchainImage ) );

  VK_CHECK( vkResetCommandBuffer( _mainCommandBuffer, 0 ) );

  // shorthand
  VkCommandBuffer cmd = _mainCommandBuffer;
  VkCommandBufferBeginInfo cmdBeginInfo = {};
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  // this command buffer will be submitted once
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  VK_CHECK( vkBeginCommandBuffer( cmd, &cmdBeginInfo ) );

  float flash = abs( sin( _frameNumber / 120.0f ) );
  VkClearValue clearValue;
  clearValue.color = { 0, flash, 0, 1 };
  std::array clearValues = { clearValue };

  VkRenderPassBeginInfo rpInfo = {};
  rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rpInfo.renderPass = _renderPass;
  rpInfo.renderArea.extent = _windowExtent;
  rpInfo.framebuffer = _framebuffers[ iSwapchainImage ];
  rpInfo.clearValueCount = ( uint32_t )clearValues.size();
  rpInfo.pClearValues = clearValues.data();

  // this will
  // - bind framebuffers
  // - clear image
  // - put image in layout specified during renderpass creation
  vkCmdBeginRenderPass( cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE );


  //VkPipeline pipeline = _selectedShader ? _trianglePipeline : _redTrianglePipeline;
  VkPipeline pipeline = _meshPipeline;
  vkCmdBindPipeline( cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline );
  //vkCmdDraw( cmd, 3, 1, 0, 0 );

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers( cmd, 0, 1, &_triangleMesh._vertexBuffer._buffer, &offset );

  const float aspect = ( float )_windowExtent.width / ( float )_windowExtent.height;
  glm::vec3 camPos = { 0,0,-2 };
  glm::mat4 view = glm::translate(glm::mat4(1), camPos);
  glm::mat4 proj = glm::perspective( glm::radians( 70.0f ), aspect, 0.1f, 200.0f );
  proj[ 1 ][ 1 ] *= -1;
  glm::mat4 model = glm::rotate( glm::mat4( 1 ), glm::radians( _frameNumber * 4.0f ), glm::vec3( 0, 1, 0 ) );
  glm::mat4 mesh_matrix = proj * view * model;
  MeshPushConstants constants;
  constants.render_matrix = mesh_matrix;
  vkCmdPushConstants( cmd,
                      _meshPipelineLayout,
                      VK_SHADER_STAGE_VERTEX_BIT,
                      0,
                      sizeof( MeshPushConstants ),
                      &constants );

  vkCmdDraw( cmd, ( uint32_t )_triangleMesh._verticies.size(), 1, 0, 0 );

  vkCmdEndRenderPass( cmd );
  VK_CHECK( vkEndCommandBuffer( cmd ) );

  // prepare submission to the queue
  // wait on the _presentSemaphore, which is signalled when the swapchain is ready
  // signal the _renderSemaphore, to signal that rendering has finished

  std::array submitWaitSemaphores = { _presentSemaphore };
  std::array submitSignalSemaphores = { _renderSemaphore };
  std::array cmdBufs = { cmd };
  VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submit = {};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  // This is hard to explain, so pls be patient
  submit.pWaitDstStageMask = &waitStage;

  submit.pWaitSemaphores = submitWaitSemaphores.data();
  submit.waitSemaphoreCount = ( uint32_t )submitWaitSemaphores.size();
  submit.pSignalSemaphores = submitSignalSemaphores.data();
  submit.signalSemaphoreCount = ( uint32_t )submitSignalSemaphores.size();
  submit.commandBufferCount = ( uint32_t )cmdBufs.size();
  submit.pCommandBuffers = cmdBufs.data();

  // submit the command buffers to the queue
  // _renderFence will now block until the graphics commands finish execution
  std::array submits = { submit };
  VK_CHECK( vkQueueSubmit( _graphicsQueue, ( uint32_t )submits.size(), submits.data(), _renderFence ) );

  std::array presentWaitSemaphores = { _renderSemaphore };
  std::array swapchains = { _swapchain };
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.swapchainCount = ( uint32_t )swapchains.size();
  presentInfo.pSwapchains = swapchains.data();
  presentInfo.waitSemaphoreCount = ( uint32_t )presentWaitSemaphores.size();
  presentInfo.pWaitSemaphores = presentWaitSemaphores.data();
  presentInfo.pImageIndices = &iSwapchainImage;
  VK_CHECK( vkQueuePresentKHR( _graphicsQueue, &presentInfo ) );

  _frameNumber++;
}

void VulkanEngine::run()
{
  SDL_Event e;
  bool bQuit = false;

  //main loop
  while( !bQuit )
  {
    //Handle events on queue
    while( SDL_PollEvent( &e ) )
    {
      switch( e.type )
      {
        case SDL_QUIT:
        {
          //close the window when user alt-f4s or clicks the X button			
          bQuit = true;
        } break;
        case SDL_KEYDOWN:
        {
          if( e.key.keysym.sym == SDLK_SPACE )
            _selectedShader = !_selectedShader;

        } break;

      }

    }

    draw();
  }
}

void VulkanEngine::init_swapchain()
{
  vkb::SwapchainBuilder swapchainBuilder( _chosenGPU, _device, _surface );
  vkb::Swapchain vkbSwapchain = swapchainBuilder
    .use_default_format_selection()
    .set_desired_present_mode( VK_PRESENT_MODE_FIFO_KHR )
    .set_desired_extent( _windowExtent.width, _windowExtent.height )
    .build()
    .value();
  _swapchain = vkbSwapchain.swapchain;

  // [ ] Q: What do i need these for?
  _swapchainImages = vkbSwapchain.get_images().value();

  _swapchainImageViews = vkbSwapchain.get_image_views().value();
  _swapchainImageFormat = vkbSwapchain.image_format;
}

void VulkanEngine::init_vulkan()
{
  uint32_t vkMajorVer = 1;
  uint32_t vkMinorVer = 1;
  vkb::InstanceBuilder builder;
  auto inst_ret = builder
    .set_app_name( "Example Vulkan Application" )
    .request_validation_layers( true )
    .require_api_version( vkMajorVer, vkMinorVer )
    .use_default_debug_messenger()
    .build();

  vkb::Instance vkb_inst = inst_ret.value();
  _instance = vkb_inst.instance;
  _debug_messenger = vkb_inst.debug_messenger;

  SDL_Vulkan_CreateSurface( _window, _instance, &_surface );
  vkb::PhysicalDeviceSelector selector( vkb_inst );
  vkb::PhysicalDevice physicalDevice = selector
    .set_minimum_version( vkMajorVer, vkMinorVer )
    .set_surface( _surface ) // grab a gpu which can render to this surface
    .select()
    .value();

  vkb::DeviceBuilder deviceBuilder( physicalDevice );
  vkb::Device vkbDevice = deviceBuilder.build().value();
  _device = vkbDevice.device;
  _chosenGPU = physicalDevice.physical_device;

  _graphicsQueue = vkbDevice.get_queue( vkb::QueueType::graphics ).value();
  _graphicsQueueFamily = vkbDevice.get_queue_index( vkb::QueueType::graphics ).value();

  // Initialize the memory allocator
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = _chosenGPU;
  allocatorInfo.device = _device;
  allocatorInfo.instance = _instance;
  vmaCreateAllocator( &allocatorInfo, &_allocator );
}

void VulkanEngine::init_commands()
{
  // this pool can submit graphics commands
  // this pool can reset individual command buffers
  VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info( _graphicsQueueFamily,
                                                                              VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT );
  VK_CHECK( vkCreateCommandPool( _device, &commandPoolInfo, nullptr, &_commandPool ) );


  VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info( _commandPool );
  VK_CHECK( vkAllocateCommandBuffers( _device, &cmdAllocInfo, &_mainCommandBuffer ) );
}

void VulkanEngine::init_default_renderpass()
{
  std::array color_attachments = { [ & ](){
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = _swapchainImageFormat;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT; // 1 sample, no msaa

    // cleared when render pass begins
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

    // rendered contents stored in memory, can be read later.
    // we want to store them so we can render it to the screen.
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // dont know/care about starting layout of attachment
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // after renderpass ends, image must be in a display layout
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    return color_attachment;
  }( ) };

  std::array color_attachment_refs = { []() {
    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    return color_attachment_ref;
  }( ) };

  std::array subpasses = { [ & ](){
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = ( uint32_t )color_attachment_refs.size();
    subpass.pColorAttachments = color_attachment_refs.data();
    return subpass;
  }( ) };


  VkRenderPassCreateInfo render_pass_info = {};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_info.attachmentCount = ( uint32_t )color_attachments.size();
  render_pass_info.pAttachments = color_attachments.data();
  render_pass_info.subpassCount = ( uint32_t )subpasses.size();
  render_pass_info.pSubpasses = subpasses.data();
  VK_CHECK( vkCreateRenderPass( _device, &render_pass_info, nullptr, &_renderPass ) );
}

void VulkanEngine::init_framebuffers()
{
  const int framebufferCount = ( int )_swapchainImageViews.size();
  _framebuffers.resize( framebufferCount );
  for( int i = 0; i < framebufferCount; ++i )
  {
    std::array attachments = { _swapchainImageViews[ i ] };
    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.renderPass = _renderPass;
    fb_info.attachmentCount = ( uint32_t )attachments.size();
    fb_info.pAttachments = attachments.data();
    fb_info.width = _windowExtent.width;
    fb_info.height = _windowExtent.height;
    fb_info.layers = 1;
    VK_CHECK( vkCreateFramebuffer( _device, &fb_info, nullptr, &_framebuffers[ i ] ) );
  }
}

void VulkanEngine::init_sync_structures()
{
  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  // we will start signalled because our VulkanEngine::draw() starts with a vkWaitForFences
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  VK_CHECK( vkCreateFence( _device, &fenceInfo, nullptr, &_renderFence ) );

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VK_CHECK( vkCreateSemaphore( _device, &semaphoreInfo, nullptr, &_presentSemaphore ) );
  VK_CHECK( vkCreateSemaphore( _device, &semaphoreInfo, nullptr, &_renderSemaphore ) );
}

void VulkanEngine::init_pipelines()
{
  struct ShaderStageCreator
  {
    VulkanEngine* _engine;
    ~ShaderStageCreator()
    {
      for( VkShaderModule shaderModule : _shaderModules )
      {
        vkDestroyShaderModule( _engine->_device, shaderModule, nullptr );
      }
    }
    bool AddModuleInfo( const char* path, VkShaderStageFlagBits stage )
    {
      VkShaderModule shaderModule = VK_NULL_HANDLE;
      if( !_engine->load_shader_module( path, &shaderModule ) )
      {
        std::cout << "failed to load shader " << path << std::endl;
        return false;
      }
      _shaderStages.push_back( vkinit::shader_stage_create_info( stage, shaderModule ) );
      return true;
    }
    void clear() { _shaderStages.clear(); }
    std::vector< VkPipelineShaderStageCreateInfo > _shaderStages;
    std::vector< VkShaderModule > _shaderModules;
  } shaderStageCreator = { this };
  if( !shaderStageCreator.AddModuleInfo( "shaders/triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT ) ||
      !shaderStageCreator.AddModuleInfo( "shaders/triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT ) )
    return;

  VkPipelineLayoutCreateInfo pipeline_layout = vkinit::pipeline_layout_create_info();
  VK_CHECK( vkCreatePipelineLayout( _device, &pipeline_layout, nullptr, &_trianglePipelineLayout ) );

  std::array pushConstants = { []()
  {
    VkPushConstantRange push_constant = {};
    push_constant.size = sizeof( MeshPushConstants );
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    return push_constant;
  }( ) };
  VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = vkinit::pipeline_layout_create_info();
  mesh_pipeline_layout_info.pPushConstantRanges = pushConstants.data();
  mesh_pipeline_layout_info.pushConstantRangeCount = ( uint32_t )pushConstants.size();
  VK_CHECK( vkCreatePipelineLayout( _device, &mesh_pipeline_layout_info, nullptr, &_meshPipelineLayout ) );

  VertexInputDescription vertexDesc = Vertex::get_vertex_description();

  PipelineBuilder pipelineBuilder = {};
  pipelineBuilder._shaderStages = shaderStageCreator._shaderStages;

  // not using atm because not reading vertexes from vertex buffers
  pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();
  pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info( VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST );
  pipelineBuilder._viewport.width = ( float )_windowExtent.width;
  pipelineBuilder._viewport.height = ( float )_windowExtent.height;
  pipelineBuilder._viewport.minDepth = 0;
  pipelineBuilder._viewport.maxDepth = 1;
  pipelineBuilder._scissor.extent = _windowExtent;
  pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info( VK_POLYGON_MODE_FILL );

  // no multisampling, run 1spp
  pipelineBuilder._multisampling = vkinit::multisample_state_create_info();

  // single blend attachment, no blend, write rgba
  pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();
  pipelineBuilder._pipelineLayout = _meshPipelineLayout;
  _redTrianglePipeline = pipelineBuilder.build_pipeline( _device, _renderPass );

  shaderStageCreator.clear();
  if( !shaderStageCreator.AddModuleInfo( "shaders/colored_triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT ) ||
      !shaderStageCreator.AddModuleInfo( "shaders/colored_triangle.vert.spv", VK_SHADER_STAGE_VERTEX_BIT ) )
    return;
  pipelineBuilder._shaderStages = shaderStageCreator._shaderStages;
  _trianglePipeline = pipelineBuilder.build_pipeline( _device, _renderPass );

  shaderStageCreator.clear();
  if( !shaderStageCreator.AddModuleInfo( "shaders/triangle_mesh.vert.spv", VK_SHADER_STAGE_VERTEX_BIT ) ||
      !shaderStageCreator.AddModuleInfo( "shaders/colored_triangle.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT ) )
    return;
  pipelineBuilder._shaderStages = shaderStageCreator._shaderStages;
  pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDesc.attributes.data();
  pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = ( uint32_t )vertexDesc.attributes.size();
  pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDesc.bindings.data();
  pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = ( uint32_t )vertexDesc.bindings.size();
  _meshPipeline = pipelineBuilder.build_pipeline( _device, _renderPass );
}

bool VulkanEngine::load_shader_module( const char* spirvpath, VkShaderModule* out )
{
  auto buf = file_to_bytes( spirvpath );
  if( buf.empty() )
    return false;
  buf.resize( round_up_nearest_multiple( ( int )buf.size(), sizeof( uint32_t ) ) );

  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = buf.size();
  createInfo.pCode = ( uint32_t* )buf.data();

  VkShaderModule shaderModule;
  VkResult res = vkCreateShaderModule( _device, &createInfo, nullptr, &shaderModule );
  if( res != VK_SUCCESS )
    return false;

  *out = shaderModule;
  return true;
}

void VulkanEngine::load_meshes()
{
  _triangleMesh._verticies.resize( 3 );
  _triangleMesh._verticies[ 0 ].position = { 1, 1, 0 };
  _triangleMesh._verticies[ 1 ].position = { -1, 1, 0 };
  _triangleMesh._verticies[ 2 ].position = { 0, -1, 0 };
  _triangleMesh._verticies[ 0 ].color = { 0, 1, 0 };
  _triangleMesh._verticies[ 1 ].color = { 0, 1, 0 };
  _triangleMesh._verticies[ 2 ].color = { 0, 1, 0 };
  upload_mesh( _triangleMesh );
}

void VulkanEngine::upload_mesh( Mesh& mesh )
{
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = mesh._verticies.size() * sizeof( Vertex );
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  VmaAllocationCreateInfo vmaAllocInfo = {};
  vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
  VK_CHECK( vmaCreateBuffer( _allocator,
                             &bufferInfo,
                             &vmaAllocInfo,
                             &mesh._vertexBuffer._buffer,
                             &mesh._vertexBuffer._allocation,
                             nullptr ) );
  // Copy data into buffer
  void* data;
  vmaMapMemory( _allocator, mesh._vertexBuffer._allocation, &data );
  memcpy( data, mesh._verticies.data(), mesh._verticies.size() * sizeof( Vertex ) );
  vmaUnmapMemory( _allocator, mesh._vertexBuffer._allocation );
}






