#include "app.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

constexpr auto HAS_GRAPHICS_QUEUE = [](std::pair<int, std::vector<IndexTypes>> pair) {
    return std::ranges::contains(pair.second, IndexTypes::GraphicsIndex);
};
constexpr auto HAS_PRESENT_QUEUE = [](std::pair<int, std::vector<IndexTypes>> pair) {
    return std::ranges::contains(pair.second, IndexTypes::PresentIndex);
};
constexpr auto HAS_COMPUTE_QUEUE = [](std::pair<int, std::vector<IndexTypes>> pair) {
    return std::ranges::contains(pair.second, IndexTypes::ComputeIndex);
};

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
    CreatePipeline();
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
    DestroyPipeline();
    m_Device.destroyShaderModule(m_VertexShader);
    m_Device.destroyShaderModule(m_FragmentShader);
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
        glfwPollEvents();
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
                m_DeviceScore.graphicsIndex,
                m_DeviceScore.graphicsIndex,
                m_SwapchainImages[imageIndex.value],
                range);

        m_DrawBuffer.pipelineBarrier2(vk::DependencyInfo({ }, nullptr, nullptr, colorTransitionBarrier));

        m_DrawBuffer.beginRendering(renderInfo);

        m_DrawBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_GraphicsPipeline);

        m_DrawBuffer.draw(3, 1, 0, 0);

        m_DrawBuffer.endRendering();

        vk::ImageMemoryBarrier2 presentTransitionBarrier(
                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                vk::AccessFlagBits2::eColorAttachmentWrite,
                vk::PipelineStageFlagBits2::eNone,
                vk::AccessFlagBits2::eNone,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::ePresentSrcKHR,
                m_DeviceScore.graphicsIndex,
                m_DeviceScore.presentIndex,
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
            while (m_Device.waitForFences(m_ExecutionFence, vk::True, 1) == vk::Result::eTimeout) { }
            DestroyPipeline();
            CreatePipeline();
            continue;
        }
        m_CurrentFrame = (m_CurrentFrame + 1) % m_SwapchainImages.size();
        glfwSwapBuffers(m_Window);
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
    if (!m_Window)
        throw std::runtime_error("Unable to create glfw window!");
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
    
    DeviceScore topScore, currentScore;
    vk::PhysicalDevice physicalDevice;

    for (const auto &device : physicalDevices)
    {
        currentScore = CheckPhysicalDevice(device);
        if (currentScore.score > topScore.score) {
            physicalDevice = device;
            topScore = currentScore;
        }
    }

    m_DeviceScore = topScore;
    m_PhysicalDevice = physicalDevice;
    std::string deviceName(m_PhysicalDevice.getProperties().deviceName);

    std::print("Chose {} as physical device\n", deviceName);

    float priority = 0.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    for (const auto &uniqueQueue : m_DeviceScore.GetUniqueQueueIndices())
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

    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features> chain = {
        deviceFeatures,
        vulkan13Features
    };

    const char *deviceExtensions[] = { vk::KHRSwapchainExtensionName };
    vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceFeatures2> deviceCreateChain = {
        vk::DeviceCreateInfo(vk::DeviceCreateFlags(), queueCreateInfos, {}, deviceExtensions),
        chain.get<vk::PhysicalDeviceFeatures2>()
    };

    VK_CHECK_AND_SET(m_Device, m_PhysicalDevice.createDevice(deviceCreateChain.get<vk::DeviceCreateInfo>()), "Unable to create logical device!");

    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Device);

    VK_CHECK_AND_SET(m_GraphicsQueue, m_Device.getQueue(m_DeviceScore.graphicsIndex, 0), "Unable to get graphics queue");
    VK_CHECK_AND_SET(m_PresentQueue, m_Device.getQueue(m_DeviceScore.presentIndex, 0), "Unable to get present queue");
    VK_CHECK_AND_SET(m_ComputeQueue, m_Device.getQueue(m_DeviceScore.computeIndex, 0), "Unable to get compute queue");
}

DeviceScore App::CheckPhysicalDevice(const vk::PhysicalDevice &device)
{
    DeviceScore deviceScore;

    vk::PhysicalDeviceProperties2 properties2 = device.getProperties2();

    if (properties2.properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu)
        deviceScore.score += 1000;

    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = device.getQueueFamilyProperties();
    int i;
    size_t numQueueFamilies = queueFamilyProperties.size();
    std::unordered_map<int, std::vector<IndexTypes>> indexTypes(numQueueFamilies);

    for (i = 0; i < numQueueFamilies; i++)
    {
        if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)
            indexTypes[i].push_back(IndexTypes::GraphicsIndex);
        if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute)
            indexTypes[i].push_back(IndexTypes::ComputeIndex);
        if (glfwGetPhysicalDevicePresentationSupport(m_Instance, device, i) == GLFW_TRUE)
            indexTypes[i].push_back(IndexTypes::PresentIndex);
    }

    auto graphicsIndicesView = indexTypes | std::views::filter(HAS_GRAPHICS_QUEUE);
    auto presentIndicesView  = indexTypes | std::views::filter(HAS_PRESENT_QUEUE);
    auto computeIndicesView  = indexTypes | std::views::filter(HAS_COMPUTE_QUEUE);

    if (graphicsIndicesView.empty() || presentIndicesView.empty() || computeIndicesView.empty())
    {
        deviceScore.score = -1;
        return deviceScore;
    }

    auto minGraphicsQueueFamily = GetNarrowestQueueFamilyIndex(graphicsIndicesView);
    auto minPresentQueueFamily = GetNarrowestQueueFamilyIndex(presentIndicesView);
    auto minComputeQueueFamily = GetNarrowestQueueFamilyIndex(computeIndicesView);

    deviceScore.score += (9 - std::clamp((int)minGraphicsQueueFamily.second, 0, 3)
                            - std::clamp((int)minPresentQueueFamily.second , 0, 3)
                            - std::clamp((int)minComputeQueueFamily.second , 0, 3)) * 10;
    
    deviceScore.graphicsIndex = minGraphicsQueueFamily.first;
    deviceScore.presentIndex = minPresentQueueFamily.first;
    deviceScore.computeIndex = minComputeQueueFamily.first;

    return deviceScore;
}

std::pair<int, size_t> App::GetNarrowestQueueFamilyIndex(auto &queueFamilyIndices)
{
    int min;
    size_t totalQueueFamilies = std::numeric_limits<int>::max(), currentQueueFamilies;
    for (const auto &pair : queueFamilyIndices)
    {
        currentQueueFamilies = pair.second.size();
        if (currentQueueFamilies < totalQueueFamilies) 
        {
            min = pair.first;
            totalQueueFamilies = currentQueueFamilies;
        }
    }

    return { min, totalQueueFamilies };
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
    vk::Format format = surfaceFormats.size() > 0? surfaceFormats[0].format : vk::Format::eUndefined;
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
    m_ColorAttachmentFormat = format;

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

    m_Viewport = vk::Viewport(0, 0, m_Settings.width, m_Settings.height, 0.0f, 1.0f);
    m_Scissor = vk::Rect2D({0, 0}, extent);
}

void App::DestroySwapchain()
{
    for (const auto &imageView : m_SwapchainImageViews)
        m_Device.destroyImageView(imageView);

    m_SwapchainImageViews.clear();
    m_SwapchainImages.clear();
}

void App::CreatePipeline() 
{
    // Setup Shader stages
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStageCreateInfos;
    shaderStageCreateInfos.push_back({{}, vk::ShaderStageFlagBits::eFragment, m_FragmentShader, "main"});
    shaderStageCreateInfos.push_back({{}, vk::ShaderStageFlagBits::eVertex, m_VertexShader, "main"});

    // Setup vertex input stage
    vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo({}, nullptr, nullptr);

    // Setup input assembly stage
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo({}, vk::PrimitiveTopology::eTriangleList, vk::False);

    // Setup tesselation state
    vk::PipelineTessellationStateCreateInfo tesselationCreateInfo({}, 3);

    // Setup viewport state
    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo({}, m_Viewport, m_Scissor);

    // Setup rasterization state
    vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo({}, vk::False, vk::False, vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise, vk::False, 0, 0, 0, 1.0);

    // Setup multisample state
    vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo({}, vk::SampleCountFlagBits::e1, vk::False);

    // Setup depth stencil state
    vk::PipelineDepthStencilStateCreateInfo depthStencilCreateInfo({}, vk::False);

    // Setup color blend state
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState(vk::True, vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

    vk::PipelineColorBlendStateCreateInfo colorBlendCreateInfo({}, vk::False, vk::LogicOp::eCopy, colorBlendAttachmentState, {1.0f, 1.0f, 1.0f, 1.0f});

    // Create pipeline layout
    vk::PipelineLayoutCreateInfo layoutCreateInfo({}, nullptr, nullptr);

    m_PipelineLayout = m_Device.createPipelineLayout(layoutCreateInfo);

    // Create rendering info for dynamic rendering
    vk::PipelineRenderingCreateInfo renderingCreateInfo(0, m_ColorAttachmentFormat);

    // Create pipeline
    vk::GraphicsPipelineCreateInfo pipelineCreateInfo({}, shaderStageCreateInfos, &vertexInputCreateInfo, &inputAssemblyCreateInfo, &tesselationCreateInfo, &viewportStateCreateInfo, &rasterizationStateCreateInfo, &multisampleStateCreateInfo, nullptr, &colorBlendCreateInfo, nullptr, m_PipelineLayout);

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineChain(pipelineCreateInfo, renderingCreateInfo);

    auto pipelineResult = m_Device.createGraphicsPipeline(nullptr, pipelineChain.get<vk::GraphicsPipelineCreateInfo>());
    switch (pipelineResult.result) 
    {
        case vk::Result::eSuccess:
            m_GraphicsPipeline = pipelineResult.value;
            break;
        default:
            std::println("Broke it");
            break;
    }
}

void App::DestroyPipeline()
{
    m_Device.destroyPipeline(m_GraphicsPipeline);
    m_Device.destroyPipelineLayout(m_PipelineLayout);
}

void App::CreateVertexBuffer()
{
    vk::BufferCreateInfo bufferCI({}, sizeof(float) * 9, vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive);

    VK_CHECK_AND_SET(m_VertexBuffer, m_Device.createBuffer(bufferCI), "Unable to create buffer");
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

    VK_CHECK_AND_SET(m_CommandPool, m_Device.createCommandPool(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_DeviceScore.graphicsIndex)), "Unable to create command pool");
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
    m_VertexShaderCode = LoadShader(vertPath);
    m_FragmentShaderCode = LoadShader(fragPath);

    auto vertexShaderCreateInfo = vk::ShaderModuleCreateInfo({ }, m_VertexShaderCode.size() * sizeof(uint32_t), m_VertexShaderCode.data());
    auto fragmentShaderCreateInfo = vk::ShaderModuleCreateInfo({ }, m_FragmentShaderCode.size() * sizeof(uint32_t), m_FragmentShaderCode.data());

    m_VertexShader = m_Device.createShaderModule(vertexShaderCreateInfo);
    m_FragmentShader = m_Device.createShaderModule(fragmentShaderCreateInfo);
}

std::vector<uint32_t> App::LoadShader(const std::string &path)
{
    std::ifstream shaderStream(path, std::ios::ate | std::ios::binary);
    if (!shaderStream.is_open())
    {
        std::print("Unable to open file: {}\n", path);
        exit(1);
    }

    size_t fileSize = (size_t) shaderStream.tellg();
    shaderStream.seekg(0);

    std::vector<uint32_t> shaderCode(fileSize / sizeof(uint32_t));

    shaderStream.read(reinterpret_cast<char *>(shaderCode.data()), fileSize);
    shaderStream.close();

    return shaderCode;
}
