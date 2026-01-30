#pragma once

#include <vector>
#include <unordered_map>

#include "vulkan/vulkan_raii.hpp"

#include <window.hpp>
#include <settings.hpp>

namespace blossom 
{
  class Blossom 
  {
    public:
      Blossom();
      ~Blossom();

      void CreateInstance();

      void SetPipeline(Pipeline shaderPipeline);
      void DrawMesh(Mesh mesh);

      template<typename T>
      void UpdateSetting(Setting setting, T value);
    private:
      void HandleResize();
      void HandleKeyEvent();

    private:
      Window m_Window;
      vk::raii::Instance m_Instance;
      Device m_Device;
      Swapchain m_Swapchain;
  };
}