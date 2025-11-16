#include "app.hpp"


int main() {
    App app;
    app.Run();
    return 0;
}

App::App()
{
    InitGLFW();
    InitVulkan();
    CreateDevice();
    CreateSurface();
    CreateSwapchain();
}

App::~App() 
{
    m_Device.destroySwapchainKHR(m_Swapchain);
    m_Device.destroy();
    m_Instance.destroySurfaceKHR(m_Surface);

    m_Instance.destroy();
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void App::Run()
{
    while (!glfwWindowShouldClose(m_Window))
    {
        glfwPollEvents();
    }
}

void App::InitGLFW()
{
    if (glfwInit() == GLFW_FALSE)
        throw std::runtime_error("Unable to initialize glfw!");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_Window = glfwCreateWindow(m_Settings.width, m_Settings.height, "Blossom", nullptr, nullptr);
}

void App::InitVulkan()
{
    vk::ApplicationInfo appInfo("Blossom", 1, nullptr, 1, vk::ApiVersion14);
    auto instanceLayers = GetInstanceLayers();
    auto instanceExtensions = GetInstanceExtensions();

    vk::InstanceCreateInfo instanceCI(
        vk::InstanceCreateFlags(), 
        &appInfo, 
        instanceLayers.size(), 
        instanceLayers.data(), 
        instanceExtensions.size(), 
        instanceExtensions.data());
    VK_CHECK_AND_SET(m_Instance, vk::createInstance(instanceCI), "Unable to create Vulkan instance");
}

std::vector<const char *> App::GetInstanceLayers()
{
    std::vector<vk::LayerProperties> layerProperties;
    std::vector<const char *> layerNames;
    VK_CHECK_AND_SET(layerProperties, vk::enumerateInstanceLayerProperties(), "Unable to enumerate instance layer properties");

#ifndef NDEBUG
    layerNames.push_back("VK_LAYER_KHRONOS_validation");
#endif

    if (!std::all_of(layerNames.begin(), layerNames.end(), [&layerProperties](const char *name)
            {
                return std::any_of(
                        layerProperties.begin(), 
                        layerProperties.end(), 
                        [&name](vk::LayerProperties const &property) { return strcmp(name, property.layerName) == 0;});
            }))
    {
        throw std::runtime_error("Unable to get required layers!");
    }

    return layerNames;
}

std::vector<const char *> App::GetInstanceExtensions()
{
    uint32_t extensionCount;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    if (!glfwExtensions)
        throw std::runtime_error("Unable to get glfw extensions!");

    std::vector<const char *> extensionNames(glfwExtensions, glfwExtensions + extensionCount);

    return extensionNames;
}

void App::CreateDevice()
{
    std::vector<vk::PhysicalDevice> physicalDevices;
    VK_CHECK_AND_SET(physicalDevices, m_Instance.enumeratePhysicalDevices(), "Unable to enumerate physical devices!");
    
    bool deviceFound = false;

    QueueFamilyInfo queueFamilyInfo;

    for (const auto &device : physicalDevices)
    {
        if (CheckPhysicalDevice(device))
        {
            m_PhysicalDevice = device;
            deviceFound = true;
            break;
        }
        m_QueueFamilyInfo.Reset();
    }

    if (!deviceFound)
        throw std::runtime_error("Unable to find suitable physical device!");

    float priority = 0.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.push_back(vk::DeviceQueueCreateInfo(
            vk::DeviceQueueCreateFlags(), 
            static_cast<uint32_t>(m_QueueFamilyInfo.graphicsIndex), 
            1, 
            &priority));
    queueCreateInfos.push_back(vk::DeviceQueueCreateInfo(
                vk::DeviceQueueCreateFlags(),
                static_cast<uint32_t>(m_QueueFamilyInfo.computeIndex),
                1,
                &priority));

    const char *deviceExtensions[] = { vk::KHRSwapchainExtensionName };
    vk::DeviceCreateInfo deviceCI(vk::DeviceCreateFlags(), queueCreateInfos, {}, deviceExtensions);
    VK_CHECK_AND_SET(m_Device, m_PhysicalDevice.createDevice(deviceCI), "Unable to create logical device!");
}

bool App::CheckPhysicalDevice(const vk::PhysicalDevice &device)
{
    if (device.getProperties().deviceType != vk::PhysicalDeviceType::eDiscreteGpu)
        return false;

    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

    for (int i = 0; i < queueFamilies.size(); i++)
    {
       if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics && glfwGetPhysicalDevicePresentationSupport(m_Instance, device, i))
           m_QueueFamilyInfo.graphicsIndex = i;
       if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute && m_QueueFamilyInfo.graphicsIndex != i) 
           m_QueueFamilyInfo.computeIndex = i;

       if (m_QueueFamilyInfo.IsComplete())
           return true;
    }

    return false;
}

void App::CreateSurface()
{
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("Unable to create glfw window surface!");
    m_Surface = surface;
}

void App::CreateSwapchain()
{
    vk::SurfaceCapabilitiesKHR surfaceCapabilities;
    std::vector<vk::SurfaceFormatKHR> surfaceFormats;
    std::vector<vk::PresentModeKHR> availablePresentModes;
    VK_CHECK_AND_SET(surfaceCapabilities, m_PhysicalDevice.getSurfaceCapabilitiesKHR(m_Surface), "Unable to get surface capabilities");
    VK_CHECK_AND_SET(surfaceFormats, m_PhysicalDevice.getSurfaceFormatsKHR(m_Surface), "Unable to get surface formats");
    VK_CHECK_AND_SET(availablePresentModes, m_PhysicalDevice.getSurfacePresentModesKHR(m_Surface), "Unable to get surface present modes");

    // Get the image format
    vk::Format format = vk::Format::eUndefined;
    for (const auto & surfaceFormat : surfaceFormats)
    {
        if (surfaceFormat.format == vk::Format::eR8G8B8A8Unorm)
        {
            format = surfaceFormat.format;
            break;
        }
    }

    if (format == vk::Format::eUndefined)
        throw std::runtime_error("Image format not supprted");

    // Setup double buffering
    uint32_t minImageCount = 2;
    if (minImageCount < surfaceCapabilities.minImageCount)
        minImageCount = surfaceCapabilities.minImageCount;
    else if (minImageCount > surfaceCapabilities.maxImageCount)
        minImageCount = surfaceCapabilities.maxImageCount;

    auto extent = surfaceCapabilities.currentExtent;
    if (extent.width == ~(uint32_t)0 && extent.height == ~(uint32_t)0)
    {
        extent.width = Clamp(m_Settings.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        extent.height = Clamp(m_Settings.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    }

    // Keep track of the surface size internally
    m_Settings.width = extent.width;
    m_Settings.height = extent.height;

    // Try to use the identity transform and fallback if needed
    auto preTransform = (surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) ? 
        vk::SurfaceTransformFlagBitsKHR::eIdentity :
        surfaceCapabilities.currentTransform;

    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
    if (std::any_of(availablePresentModes.begin(), 
                    availablePresentModes.end(), 
                    [] (vk::PresentModeKHR const &presentMode) { return presentMode == vk::PresentModeKHR::eMailbox; }))
        presentMode = vk::PresentModeKHR::eMailbox;

    vk::SwapchainCreateInfoKHR swapchainCI(
            vk::SwapchainCreateFlagsKHR(),
            m_Surface,
            minImageCount,
            format,
            vk::ColorSpaceKHR::eSrgbNonlinear,
            extent,
            1,
            vk::ImageUsageFlagBits::eColorAttachment,
            vk::SharingMode::eExclusive,
            { },
            preTransform,
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            presentMode,
            vk::True,
            nullptr);
    VK_CHECK_AND_SET(m_Swapchain, m_Device.createSwapchainKHR(swapchainCI), "Unable to create swapchain");
}

