#include "vk_pipeline.h"
#include <array>
#include <iostream>

VkPipeline PipelineBuilder::build_pipeline( VkDevice device, VkRenderPass pass )
{
  std::array viewports = { _viewport };
  std::array scissors = { _scissor };
  std::array attachments = { _colorBlendAttachment };

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = ( int )viewports.size();
  viewportState.pViewports = viewports.data();
  viewportState.pScissors = scissors.data();
  viewportState.scissorCount = ( int )scissors.size();

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.attachmentCount = ( int )attachments.size();
  colorBlending.pAttachments = attachments.data();

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = ( int )_shaderStages.size();
  pipelineInfo.pStages = _shaderStages.data();
  pipelineInfo.pVertexInputState = &_vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &_inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &_rasterizer;
  pipelineInfo.pMultisampleState = &_multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.layout = _pipelineLayout;
  pipelineInfo.renderPass = pass;

  std::array pipelineInfos = { pipelineInfo };
  VkPipeline pipeline;
  if( VK_SUCCESS == vkCreateGraphicsPipelines( device,
                                               VK_NULL_HANDLE,
                                               ( int )pipelineInfos.size(),
                                               pipelineInfos.data(),
                                               nullptr,
                                               &pipeline ) )
    return pipeline;

  std::cout << "Failed to create pipieline" << std::endl;
  return VK_NULL_HANDLE;
}

