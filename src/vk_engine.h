﻿// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>

#include <vector>

class VulkanEngine
{
public:

  void init();
  void cleanup();
  void draw();
  void run();

  // Basic windowing
  bool _isInitialized = false;
  int _frameNumber = 0;
  VkExtent2D _windowExtent = { 1700 , 900 };
  struct SDL_Window* _window = nullptr;

  // Core vulkan structures
  VkInstance _instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT _debug_messenger = VK_NULL_HANDLE;
  VkPhysicalDevice _chosenGPU = VK_NULL_HANDLE;
  VkDevice _device = VK_NULL_HANDLE;
  VkSurfaceKHR _surface = VK_NULL_HANDLE;

  // Swapchain
  VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
  VkFormat _swapchainImageFormat = VK_FORMAT_UNDEFINED;
  std::vector< VkImage > _swapchainImages;
  std::vector< VkImageView > _swapchainImageViews;

  // Command submission
  VkQueue _graphicsQueue = VK_NULL_HANDLE;
  uint32_t _graphicsQueueFamily = -1;
  VkCommandPool _commandPool = VK_NULL_HANDLE;
  VkCommandBuffer _mainCommandBuffer = VK_NULL_HANDLE;

  // Render pass
  VkRenderPass _renderPass;
  std::vector< VkFramebuffer > _framebuffers;

  // Main loop
  VkSemaphore _presentSemaphore = VK_NULL_HANDLE;
  VkSemaphore _renderSemaphore = VK_NULL_HANDLE;
  VkFence _renderFence = VK_NULL_HANDLE;

private:

  void init_vulkan();
  void init_swapchain();
  void init_commands();
  void init_default_renderpass();
  void init_framebuffers();
  void init_sync_structures();
};