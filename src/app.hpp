#ifndef APP_HPP
#define APP_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>

class HelloTriangleApplication
{
public:
    void Run();

private:
    void InitWindow();

    void InitVulkan();

    void MainLoop();

    void Cleanup();

    void CreateInstance();

    bool CheckValidationLayerSupport();

    std::vector<const char *> GetRequiredExtensions();
    
    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    void SetupDebugMessenger();

private:
    GLFWwindow *m_Window;

    VkInstance m_Instance;
    VkDebugUtilsMessengerEXT m_DebugMessenger;
    
    static constexpr uint32_t WINDOW_WIDTH = 800;
    static constexpr uint32_t WINDOW_HEIGHT = 600;

    const std::vector<const char *> m_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

#if NDEBUG
    const bool m_EnableValidationLayers = false;
#else
    const bool m_EnableValidationLayers = true;
#endif
};

#endif
