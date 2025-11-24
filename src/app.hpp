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

struct QueueFamilyInfo
{
    int32_t graphicsIndex;
    int32_t presentIndex;
    int32_t computeIndex;

    QueueFamilyInfo() 
    {
        Reset();
    }

    void Reset()
    {
        graphicsIndex = -1;
        presentIndex = -1;
        computeIndex = -1;
    }

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
    bool CheckPhysicalDevice(const vk::PhysicalDevice &device);
    
    // Swapchain and surface creation
    void CreateSurface();
    void CreateSwapchain();
    void DestroySwapchain();

    void CreateShaders(const std::string &fragPath, const std::string &vertPath);
    vk::ShaderCreateInfoEXT GetShaderCreateInfo(
            vk::ShaderStageFlagBits stage,
            vk::ShaderStageFlagBits nextStage,
            std::string &shaderCode);
    std::string LoadShader(const std::string &path);

    void CreateVertexBuffer();

    // Draw setup
    void SetupDraw();

    static void OnResize(GLFWwindow *window, int width, int height);

private:
    GLFWwindow *m_Window;
    Settings m_Settings;
    vk::Instance m_Instance;
    vk::PhysicalDevice m_PhysicalDevice;
    QueueFamilyInfo m_QueueFamilyInfo;
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
    vk::CommandPool m_CommandPool;
    vk::CommandBuffer m_DrawBuffer;
    vk::Fence m_ExecutionFence;
    uint32_t m_CurrentFrame;
    bool m_WindowResized;
    vk::ShaderEXT m_VertexShader;
    vk::ShaderEXT m_FragmentShader;
    vk::Buffer m_VertexBuffer;
    vk::Viewport m_Viewport;
    vk::Rect2D m_Scissor;
};

