#pragma once

#include "vulkan/vulkan_raii.hpp"

namespace blossom 
{
  class Device
  {
    private:
      Device();

      vk::raii::PhysicalDevice m_PhysicalDevice;
      vk::raii::Device m_LogicalDevice;
  };
}