#pragma once

#include <GLFW/glfw3.h>

#include "vulkan/vulkan_raii.hpp"

namespace blossom 
{
  class Window 
  {
    public:
      Window();
      ~Window();

      vk::raii::SurfaceKHR GetSurface(vk::raii::Instance &instance);

    private:
      void OnResize(GLFWwindow *window, int width, int height);

    private:
      GLFWwindow *m_GLFWWindow;

      vk::raii::SurfaceKHR m_Surface;

      uint32_t m_Width;
      uint32_t m_Height;

      bool m_isFullscreen;
  };
}