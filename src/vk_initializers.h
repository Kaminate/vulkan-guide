// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>

namespace vkinit
{

  VkCommandPoolCreateInfo command_pool_create_info( uint32_t queueFamilyIndex,
                                                    VkCommandPoolCreateFlags flags = 0 );

  // Primary buffers are sent to the vkqueue, secondary buffers send subcommands to the primary buffer 
  VkCommandBufferAllocateInfo command_buffer_allocate_info( VkCommandPool pool,
                                                            uint32_t count = 1,
                                                            VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY );

  VkPipelineShaderStageCreateInfo shader_stage_create_info( VkShaderStageFlagBits, VkShaderModule );
  VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();
  VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info( VkPrimitiveTopology );
  VkPipelineRasterizationStateCreateInfo rasterization_state_create_info( VkPolygonMode );
  VkPipelineMultisampleStateCreateInfo multisample_state_create_info();
  VkPipelineColorBlendAttachmentState color_blend_attachment_state();
  VkPipelineLayoutCreateInfo pipeline_layout_create_info();
}

