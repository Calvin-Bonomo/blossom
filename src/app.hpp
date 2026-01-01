#pragma once

#include "vulkan/vulkan.hpp"
#include <GLFW/glfw3.h>

#include "settings.hpp"
#include "utils.hpp"

#include <vector>
#include <set>
#include <exception>
#include <print>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <limits>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <ranges>

enum class IndexTypes {
    GraphicsIndex,
    PresentIndex,
    ComputeIndex
};

struct DeviceScore
{
    int32_t graphicsIndex = -1;
    int32_t presentIndex = -1;
    int32_t computeIndex = -1; 
    int32_t score = 0;

    bool IsComplete() 
    {
        return graphicsIndex >= 0 && presentIndex >= 0 && computeIndex >= 0;
    }

    std::set<int32_t> GetUniqueQueueIndices()
    {
        int32_t uniqueIndices[] = { graphicsIndex, presentIndex, computeIndex };
        return std::set<int32_t>(uniqueIndices, uniqueIndices + 3);
    }
};

class App {
public:
    App();
    ~App();

    void Run();
private:
    void InitGLFW();
    
    // Instance Initialization
    void InitVulkan();
    std::vector<const char *> GetInstanceLayers();
    std::vector<const char *> GetInstanceExtensions();

    // Device Initialization
    void CreateDevice();
    DeviceScore CheckPhysicalDevice(const vk::PhysicalDevice &device);
    std::pair<int, size_t> GetNarrowestQueueFamilyIndex(auto &queueFamilyIndices);
    
    // Swapchain and surface creation
    void CreateSurface();
    void CreateSwapchain();
    void DestroySwapchain();

    void CreateShaders(const std::string &fragPath, const std::string &vertPath);
    std::vector<uint32_t> LoadShader(const std::string &path);

    void CreatePipeline();
    void DestroyPipeline();

    void CreateVertexBuffer();

    // Draw setup
    void SetupDraw();

    static void OnResize(GLFWwindow *window, int width, int height);

private:
    GLFWwindow *m_Window;
    Settings m_Settings;
    vk::Instance m_Instance;
    vk::PhysicalDevice m_PhysicalDevice;
    DeviceScore m_DeviceScore;
    vk::Device m_Device;
    vk::SurfaceKHR m_Surface;
    vk::SwapchainKHR m_Swapchain;
    vk::Queue m_GraphicsQueue;
    vk::Queue m_PresentQueue;
    vk::Queue m_ComputeQueue;
    std::vector<vk::Semaphore> m_AcquireFrameSemaphores;
    std::vector<vk::Semaphore> m_ReleaseFrameSemaphores;
    std::vector<vk::Image> m_SwapchainImages;
    std::vector<vk::ImageView> m_SwapchainImageViews;
    vk::Format m_ColorAttachmentFormat;
    vk::CommandPool m_CommandPool;
    vk::CommandBuffer m_DrawBuffer;
    vk::Fence m_ExecutionFence;
    uint32_t m_CurrentFrame;
    bool m_WindowResized;
    std::vector<uint32_t> m_VertexShaderCode;
    std::vector<uint32_t> m_FragmentShaderCode;
    vk::ShaderModule m_VertexShader;
    vk::ShaderModule m_FragmentShader;
    vk::Buffer m_VertexBuffer;
    vk::Viewport m_Viewport;
    vk::Rect2D m_Scissor;
    vk::PipelineLayout m_PipelineLayout;
    vk::Pipeline m_GraphicsPipeline;
};

