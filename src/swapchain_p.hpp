#pragma once

#include "vulkan/vulkan.hpp"

namespace blossom
{
    struct Swapchain 
    {
        vk::SurfaceKHR m_WindowSurface;
        vk::SwapchainKHR m_Swapchain;
        // color attachment color format
        // depth stencil attachment format
    };
}
