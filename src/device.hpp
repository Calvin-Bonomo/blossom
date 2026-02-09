#pragma once

#include "vulkan/vulkan.hpp"

namespace blossom 
{
    class Device
    {
    private:
        Device();

        vk::PhysicalDevice m_PhysicalDevice;
        vk::Device m_LogicalDevice;
    };
}
