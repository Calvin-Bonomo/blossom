#pragma once

#include "vulkan/vulkan_raii.hpp"

namespace blossom
{
  struct Swapchain {
    vk::raii::SwapchainKHR m_Swapchain;
    // color attachment color format
    // depth stencil attachment format
  };
}