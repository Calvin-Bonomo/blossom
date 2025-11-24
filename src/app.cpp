#include "app.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

int main() {
    App app;
    app.Run();
    return 0;
}

App::App()
    : m_WindowResized(false)
{
    InitGLFW();
    InitVulkan();
    CreateDevice();
    CreateSurface();
    CreateSwapchain();
    CreateShaders("res/vert.spv", "res/frag.spv");
    SetupDraw();
}

App::~App() 
{
    m_Device.destroyFence(m_ExecutionFence);
    m_Device.destroyCommandPool(m_CommandPool);
    for (const auto &semaphore : m_AcquireFrameSemaphores)
        m_Device.destroySemaphore(semaphore);
    for (const auto &semaphore : m_ReleaseFrameSemaphores)
        m_Device.destroySemaphore(semaphore);
    m_Device.destroyShaderEXT(m_VertexShader);
    m_Device.destroyShaderEXT(m_FragmentShader);
    for (const auto &imageView : m_SwapchainImageViews)
        m_Device.destroyImageView(imageView);
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
        while (m_Device.waitForFences(m_ExecutionFence, vk::True, 0) == vk::Result::eTimeout);

        vk::ResultValue<uint32_t> imageIndex = m_Device.acquireNextImageKHR(m_Swapchain, UINT64_MAX, m_AcquireFrameSemaphores[m_CurrentFrame]);
        if (imageIndex.result == vk::Result::eSuboptimalKHR || imageIndex.result == vk::Result::eErrorOutOfDateKHR)
        {
            std::print("Swapchain out of date! Recreating...\n");
            DestroySwapchain();
            CreateSwapchain();
            continue;
        }

        m_DrawBuffer.reset();
        m_Device.resetFences(m_ExecutionFence);

        vk::RenderingAttachmentInfo colorAttachmentInfo(
                m_SwapchainImageViews[imageIndex.value], 
                vk::ImageLayout::eColorAttachmentOptimal, 
                vk::ResolveModeFlagBits::eNone,
                nullptr,
                vk::ImageLayout::eUndefined,
                vk::AttachmentLoadOp::eClear,
                vk::AttachmentStoreOp::eStore,
                vk::ClearValue({0.0f, 0.0f, 0.0f, 1.0f}));
        vk::Rect2D renderArea({0, 0}, {m_Settings.width, m_Settings.height});
        vk::RenderingInfo renderInfo({ }, renderArea, 1, 0, colorAttachmentInfo);

        vk::ImageSubresourceRange range(
                vk::ImageAspectFlagBits::eColor, 
                0, 
                vk::RemainingMipLevels,
                0,
                vk::RemainingArrayLayers);

        m_DrawBuffer.begin(vk::CommandBufferBeginInfo());

        vk::ImageMemoryBarrier2 colorTransitionBarrier(
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                vk::AccessFlagBits2::eColorAttachmentWrite,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eColorAttachmentOptimal,
                m_QueueFamilyInfo.graphicsIndex,
                m_QueueFamilyInfo.graphicsIndex,
                m_SwapchainImages[imageIndex.value],
                range);

        m_DrawBuffer.pipelineBarrier2(vk::DependencyInfo({ }, nullptr, nullptr, colorTransitionBarrier));

        m_DrawBuffer.beginRendering(renderInfo);

        m_DrawBuffer.endRendering();

        vk::ImageMemoryBarrier2 presentTransitionBarrier(
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                vk::AccessFlagBits2::eColorAttachmentWrite,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::ePresentSrcKHR,
                m_QueueFamilyInfo.graphicsIndex,
                m_QueueFamilyInfo.presentIndex,
                m_SwapchainImages[imageIndex.value],
                range);

        m_DrawBuffer.pipelineBarrier2(vk::DependencyInfo({ }, nullptr, nullptr, presentTransitionBarrier));

        m_DrawBuffer.end();

        vk::SemaphoreSubmitInfo waitSemaphoreSubmitInfo(m_AcquireFrameSemaphores[m_CurrentFrame], 0, vk::PipelineStageFlagBits2::eTopOfPipe);
        vk::SemaphoreSubmitInfo signalSemaphoreSubmitInfo(m_ReleaseFrameSemaphores[m_CurrentFrame], 0, vk::PipelineStageFlagBits2::eBottomOfPipe);
        vk::CommandBufferSubmitInfo commandBufferSubmitInfo(m_DrawBuffer);

        vk::SubmitInfo2 submitInfo({ }, waitSemaphoreSubmitInfo, commandBufferSubmitInfo, signalSemaphoreSubmitInfo);

        m_GraphicsQueue.submit2(submitInfo, m_ExecutionFence);

        vk::Result result = m_PresentQueue.presentKHR(vk::PresentInfoKHR(m_ReleaseFrameSemaphores[m_CurrentFrame], m_Swapchain, imageIndex.value));
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_WindowResized)
        {
            m_WindowResized = false;
            DestroySwapchain();
            CreateSwapchain();
            continue;
        }
        m_CurrentFrame = (m_CurrentFrame + 1) % m_SwapchainImages.size();
        glfwPollEvents();
    }

    m_Device.waitIdle();
}

void App::InitGLFW()
{
    if (glfwInit() == GLFW_FALSE)
        throw std::runtime_error("Unable to initialize glfw!");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);

    m_Window = glfwCreateWindow(m_Settings.width, m_Settings.height, "Blossom", nullptr, nullptr);
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetWindowSizeCallback(m_Window, OnResize);
}

void App::InitVulkan()
{
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

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

    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Instance);
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
    for (const auto &uniqueQueue : m_QueueFamilyInfo.GetUniqueQueueIndices())
    {
        queueCreateInfos.push_back(
                vk::DeviceQueueCreateInfo(
                    { },
                    static_cast<uint32_t>(uniqueQueue),
                    1,
                    &priority));
    }

    vk::PhysicalDeviceVulkan13Features vulkan13Features;
    vulkan13Features.dynamicRendering = vk::True;
    vulkan13Features.synchronization2 = vk::True;

    vk::PhysicalDeviceFeatures2 deviceFeatures;

    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceShaderObjectFeaturesEXT> chain = {
        deviceFeatures,
        vulkan13Features,
        vk::PhysicalDeviceShaderObjectFeaturesEXT(vk::True)
    };

    const char *deviceExtensions[] = { vk::KHRSwapchainExtensionName, vk::EXTShaderObjectExtensionName };
    vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceFeatures2> deviceCreateChain = {
        vk::DeviceCreateInfo(vk::DeviceCreateFlags(), queueCreateInfos, {}, deviceExtensions),
        chain.get<vk::PhysicalDeviceFeatures2>()
    };

    VK_CHECK_AND_SET(m_Device, m_PhysicalDevice.createDevice(deviceCreateChain.get<vk::DeviceCreateInfo>()), "Unable to create logical device!");

    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Device);

    VK_CHECK_AND_SET(m_GraphicsQueue, m_Device.getQueue(m_QueueFamilyInfo.graphicsIndex, 0), "Unable to get graphics queue");
    VK_CHECK_AND_SET(m_PresentQueue, m_Device.getQueue(m_QueueFamilyInfo.presentIndex, 0), "Unable to get present queue");
    VK_CHECK_AND_SET(m_ComputeQueue, m_Device.getQueue(m_QueueFamilyInfo.computeIndex, 0), "Unable to get compute queue");
}

bool App::CheckPhysicalDevice(const vk::PhysicalDevice &device)
{
    if (device.getProperties().deviceType != vk::PhysicalDeviceType::eDiscreteGpu)
        return false;

    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

    for (int i = 0; i < queueFamilies.size(); i++)
    {
       if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)
           m_QueueFamilyInfo.graphicsIndex = i;
       if (glfwGetPhysicalDevicePresentationSupport(m_Instance, device, i)) 
           m_QueueFamilyInfo.presentIndex = i;
       if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute)
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
    if (extent.width == std::numeric_limits<uint32_t>::max())
    {
        int width, height;
        glfwGetFramebufferSize(m_Window, &width, &height);
        extent.width = std::clamp(static_cast<uint32_t>(width), surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        extent.height = std::clamp(static_cast<uint32_t>(height), surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
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

    vk::SwapchainKHR oldSwapchain = nullptr;
    if (m_Swapchain != nullptr)
        oldSwapchain = m_Swapchain;

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
            oldSwapchain); 
    VK_CHECK_AND_SET(m_Swapchain, m_Device.createSwapchainKHR(swapchainCI), "Unable to create swapchain");

    if (oldSwapchain != nullptr)
        m_Device.destroySwapchainKHR(oldSwapchain);

    VK_CHECK_AND_SET(m_SwapchainImages, m_Device.getSwapchainImagesKHR(m_Swapchain), "Unable to get swapchain images");
    vk::ImageSubresourceRange range(
            vk::ImageAspectFlagBits::eColor, 
            0, 
            vk::RemainingMipLevels, 
            0, 
            vk::RemainingArrayLayers);
    vk::ImageView imageView;
    for (const auto &image : m_SwapchainImages)
    {
        vk::ImageViewCreateInfo imageViewCI({ }, image, vk::ImageViewType::e2D, format, { }, range);
        VK_CHECK_AND_SET(imageView, m_Device.createImageView(imageViewCI), "Unable to create image view");
        m_SwapchainImageViews.push_back(imageView);
    }
}

void App::DestroySwapchain()
{
    for (const auto &imageView : m_SwapchainImageViews)
        m_Device.destroyImageView(imageView);

    m_SwapchainImageViews.clear();
    m_SwapchainImages.clear();
}

void App::SetupDraw()
{
    m_CurrentFrame = 0;

    for (int i = 0; i < m_SwapchainImages.size(); i++)
    {
        vk::Semaphore semaphore;
        VK_CHECK_AND_SET(semaphore, m_Device.createSemaphore({ }), "Unable to create semaphore");
        m_AcquireFrameSemaphores.push_back(semaphore);
        VK_CHECK_AND_SET(semaphore, m_Device.createSemaphore({ }), "Unable to create semaphore");
        m_ReleaseFrameSemaphores.push_back(semaphore);
    }

    VK_CHECK_AND_SET(m_CommandPool, m_Device.createCommandPool(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_QueueFamilyInfo.graphicsIndex)), "Unable to create command pool");
    vk::CommandBufferAllocateInfo commandBufferAllocateInfo(m_CommandPool, vk::CommandBufferLevel::ePrimary, 1);
    VK_CHECK_AND_SET(m_DrawBuffer, m_Device.allocateCommandBuffers(commandBufferAllocateInfo).front(), "Unable to allocate command buffers");
    VK_CHECK_AND_SET(m_ExecutionFence, m_Device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)), "Failed to create execution fence");
}

void App::OnResize(GLFWwindow *window, int width, int height)
{
    App *pBlossom = static_cast<App *>(glfwGetWindowUserPointer(window));
    pBlossom->m_WindowResized = true;
}

void App::CreateShaders(const std::string &vertPath, const std::string &fragPath)
{
    auto vertexShader = LoadShader(vertPath);
    auto fragmentShader = LoadShader(fragPath);
    std::vector<vk::ShaderCreateInfoEXT> shaderCreateInfos;
    
    shaderCreateInfos.push_back(
            GetShaderCreateInfo(
                vk::ShaderStageFlagBits::eVertex, 
                vk::ShaderStageFlagBits::eFragment, 
                vertexShader));
    shaderCreateInfos.push_back(
            GetShaderCreateInfo(
                vk::ShaderStageFlagBits::eFragment, 
                (vk::ShaderStageFlagBits)0, 
                fragmentShader));

    auto results = m_Device.createShadersEXT(shaderCreateInfos);
    switch (results.result)
    {
        case vk::Result::eSuccess:
            break;
        case vk::Result::eErrorIncompatibleShaderBinaryEXT:
            std::print("Incompatible shader binary!\n");
            exit(1);
        default:
            std::print("Unable to create shaders\n");
            exit(1);
    }
    auto shaders = results.value;
    m_VertexShader = shaders[0];
    m_FragmentShader = shaders[1];
}

vk::ShaderCreateInfoEXT App::GetShaderCreateInfo(
        vk::ShaderStageFlagBits stage,
        vk::ShaderStageFlagBits nextStage,
        std::string &shaderCode)
{
    vk::ShaderCreateInfoEXT shaderCreateInfo(
            {}, 
            stage, 
            nextStage,
            vk::ShaderCodeTypeEXT::eSpirv, 
            shaderCode.size(), 
            shaderCode.c_str(), 
            "main");

    return shaderCreateInfo;
}

std::string App::LoadShader(const std::string &path)
{
    char buf[1024]; // we could clear this but it doesnt matter
    std::string shaderCode;
    std::ifstream shaderStream(path);
    if (!shaderStream.is_open())
    {
        std::print("Unable to open file: {}\n", path);
        exit(1);
    }

    int charsRead;
    while (!shaderStream.eof())
    {
        shaderStream.read(buf, 1024);
        charsRead = shaderStream.gcount();
        shaderCode.append(buf, charsRead);
    }
    return shaderCode;
}
