#ifndef APP_HPP
#define APP_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>

struct QueueFamilyIndices 
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

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

    void PickPhysicalDevice();
    bool IsDeviceSuitable(VkPhysicalDevice device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    void CreateLogicalDevice();

    void CreateSurface();
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
    void CreateSwapChain();
    void CreateImageViews();

private:
    GLFWwindow *m_Window;

    VkInstance m_Instance;
    VkDebugUtilsMessengerEXT m_DebugMessenger;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device;
    VkQueue m_GraphicsQueue;

    VkSurfaceKHR m_Surface;
    VkQueue m_PresentQueue;

    VkSwapchainKHR m_Swapchain;

    std::vector<VkImage> m_SwapchainImages;
    VkFormat m_SwapchainImageFormat;
    VkExtent2D m_SwapchainExtent;

    std::vector<VkImageView> m_SwapchainImageViews;
    
    static constexpr uint32_t WINDOW_WIDTH = 800;
    static constexpr uint32_t WINDOW_HEIGHT = 600;

    const std::vector<const char *> m_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

    const std::vector<const char *> m_DeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

#if NDEBUG
    const bool m_EnableValidationLayers = false;
#else
    const bool m_EnableValidationLayers = true;
#endif
};

#endif
