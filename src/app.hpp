#pragma once

import vulkan_hpp;

#include <GLFW/glfw3.h>

#include <vector>

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
    bool CheckPhysicalDevice(const vk::PhysicalDevice &device, QueueFamilyInfo &queueFamilyInfo);
private:
    GLFWwindow *m_Window;
    vk::Instance m_Instance;
    vk::PhysicalDevice m_PhysicalDevice;
    vk::Device m_Device;
};

