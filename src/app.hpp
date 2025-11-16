#pragma once

#include "vulkan/vulkan.hpp"
#include <GLFW/glfw3.h>

#include "settings.hpp"
#include "utils.hpp"

#include <vector>
#include <exception>
#include <print>
#include <cstdlib>
#include <algorithm>
#include <cstring>

struct QueueFamilyInfo
{
    int32_t graphicsIndex;
    int32_t computeIndex;

    QueueFamilyInfo(): graphicsIndex(-1), computeIndex(-1) { }

    void Reset()
    {
        graphicsIndex = -1;
        computeIndex = -1;
    }

    bool IsComplete()
    {
        return graphicsIndex >= 0 && computeIndex >= 0;
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

    void CreateSurface();
    void CreateSwapchain();
private:
    GLFWwindow *m_Window;
    Settings m_Settings;
    vk::Instance m_Instance;
    vk::PhysicalDevice m_PhysicalDevice;
    QueueFamilyInfo m_QueueFamilyInfo;
    vk::Device m_Device;
    vk::SurfaceKHR m_Surface;
    vk::SwapchainKHR m_Swapchain;
};

