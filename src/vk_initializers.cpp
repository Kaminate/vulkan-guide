#include <vk_initializers.h>

VkCommandPoolCreateInfo vkinit::command_pool_create_info( uint32_t queueFamilyIndex,
                                                          VkCommandPoolCreateFlags flags )
{
  VkCommandPoolCreateInfo commandPoolInfo = {};
  commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolInfo.queueFamilyIndex = queueFamilyIndex;
  commandPoolInfo.flags = flags;
  return commandPoolInfo;
}

VkCommandBufferAllocateInfo vkinit::command_buffer_allocate_info( VkCommandPool pool,
                                                                  uint32_t count,
                                                                  VkCommandBufferLevel level )
{
  VkCommandBufferAllocateInfo cmdAllocInfo = {};
  cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdAllocInfo.commandPool = pool;
  cmdAllocInfo.commandBufferCount = count;
  cmdAllocInfo.level = level;
  return cmdAllocInfo;
}

VkPipelineShaderStageCreateInfo vkinit::shader_stage_create_info( VkShaderStageFlagBits stage,
                                                                  VkShaderModule shaderModule )
{
  VkPipelineShaderStageCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info.stage = stage;
  info.module = shaderModule;
  info.pName = "main";
  return info;
}

VkPipelineVertexInputStateCreateInfo vkinit::vertex_input_state_create_info()
{
  VkPipelineVertexInputStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  return info;
}

VkPipelineInputAssemblyStateCreateInfo vkinit::input_assembly_create_info( VkPrimitiveTopology topology )
{
  VkPipelineInputAssemblyStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  info.topology = topology;
  return info;
}

VkPipelineRasterizationStateCreateInfo vkinit::rasterization_state_create_info( VkPolygonMode polygonMode )
{
  VkPipelineRasterizationStateCreateInfo info = { };
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  info.polygonMode = polygonMode;
  info.lineWidth = 1;
  info.cullMode = VK_CULL_MODE_NONE;
  info.frontFace = VK_FRONT_FACE_CLOCKWISE;
  return info;
}

VkPipelineMultisampleStateCreateInfo vkinit::multisample_state_create_info()
{
  VkPipelineMultisampleStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // 1 sample per pixel ( no multisampling )
  info.minSampleShading = 1; // [ ] ?
  return info;
}

VkPipelineColorBlendAttachmentState vkinit::color_blend_attachment_state()
{
  VkPipelineColorBlendAttachmentState info = {};
  info.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT |
    VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT |
    VK_COLOR_COMPONENT_A_BIT;
  return info;
}

VkPipelineLayoutCreateInfo vkinit::pipeline_layout_create_info()
{
  VkPipelineLayoutCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  return info;
}

VkPipelineDepthStencilStateCreateInfo vkinit::depth_stencil_create_info( bool depthTest,
                                                                 bool depthWrite,
                                                                 VkCompareOp compareOp )
{
  VkPipelineDepthStencilStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  info.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
  info.depthWriteEnable = depthWrite ? VK_TRUE : VK_FALSE;
  info.depthCompareOp = depthTest ? compareOp : VK_COMPARE_OP_ALWAYS;
  info.minDepthBounds = 0;
  info.maxDepthBounds = 1;
  info.stencilTestEnable = VK_FALSE;
  return info;
}

VkImageCreateInfo vkinit::image_create_info( VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent )
{
  VkImageCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  info.extent = extent;
  info.format = format;
  info.imageType = VK_IMAGE_TYPE_2D;
  info.mipLevels = 1;
  info.arrayLayers = 1;
  info.samples = VK_SAMPLE_COUNT_1_BIT;
  info.tiling = VK_IMAGE_TILING_OPTIMAL;
  info.usage = usageFlags;
  return info;
}

VkImageViewCreateInfo vkinit::image_view_create_info( VkFormat format, VkImage image, VkImageAspectFlags aspectFlags )
{
  VkImageViewCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  info.image = image;
  info.format = format;
  info.subresourceRange.levelCount = 1;
  info.subresourceRange.layerCount = 1;
  info.subresourceRange.aspectMask = aspectFlags;
  return info;
}

VkRenderPassBeginInfo vkinit::renderpass_begin_info( VkRenderPass renderPass,
                                                     VkExtent2D extent,
                                                     VkFramebuffer framebuffer )
{
  VkRenderPassBeginInfo rpInfo = {};
  rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rpInfo.renderPass = renderPass;
  rpInfo.renderArea.extent = extent;
  rpInfo.framebuffer = framebuffer;
  return rpInfo;
}
