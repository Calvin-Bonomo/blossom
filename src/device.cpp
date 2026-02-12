#include "device_p.hpp"

#include <tuple>
#include <unordered_map>
#include <bit>
#include <ranges>
#include <limits>
#include <print>
#include <set>

#include "GLFW/glfw3.h"

#include "error_p.hpp"

namespace blossom
{
    using QueueFamilyMask = uint8_t;

    enum class QueueFamilyFlag : uint8_t
    {
        Graphics = 0x1,
        Present = 0x2,
        Compute = 0x4
    };

    auto isGraphicsQueue = [] (std::pair<int, QueueFamilyMask> pair) {
        return pair.second & static_cast<QueueFamilyMask>(QueueFamilyFlag::Graphics);
    };

    auto isPresentQueue = [] (std::pair<int, QueueFamilyMask> pair) {
        return pair.second & static_cast<QueueFamilyMask>(QueueFamilyFlag::Present);
    };

    auto isComputeQueue = [] (std::pair<int, QueueFamilyMask> pair) {
        return pair.second & static_cast<QueueFamilyMask>(QueueFamilyFlag::Compute);
    };

    std::pair<int, int> GetNarrowestQueueFamilyIndex(std::ranges::input_range auto &&queueFamilyIndices);
}

// Get the queue family index with the fewest number of other queues present
std::pair<int, int> blossom::GetNarrowestQueueFamilyIndex(std::ranges::input_range auto &&queueFamilyIndices)
{
    if (queueFamilyIndices.empty()) return { -1, -1 };

    int minQueueFamilyIndex, currentQueueFamilyCount, minQueueFamilyCount = std::numeric_limits<int>::max();
    for (const auto &pair : queueFamilyIndices)
    {
        currentQueueFamilyCount = std::popcount(static_cast<QueueFamilyMask>(pair.second));
        if (currentQueueFamilyCount < minQueueFamilyCount) 
        {
            minQueueFamilyIndex = pair.first;
            minQueueFamilyCount = currentQueueFamilyCount;
        }
    }

    return { minQueueFamilyIndex, minQueueFamilyCount };
}

blossom::Device::PhysicalDeviceScore blossom::Device::GetDeviceScore(vk::Instance instance, vk::PhysicalDevice physicalDevice) 
{
    PhysicalDeviceScore deviceScore;

    vk::PhysicalDeviceProperties2 properties2 = physicalDevice.getProperties2();
    // TODO: Update to getQueueFamilyProperties2
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    // Reward discrete GPUs so we don't choose integrated graphics if possible
    if (properties2.properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu)
        deviceScore.score += 1000;

    size_t numQueueFamilies = queueFamilyProperties.size();
    std::unordered_map<int, QueueFamilyMask> queueFamilyQueueTypes(numQueueFamilies);

    for (int i = 0; i < numQueueFamilies; i++)
    {
        QueueFamilyMask mask = 0;
        if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)
            mask |= static_cast<QueueFamilyMask>(QueueFamilyFlag::Graphics);
        if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute)
            mask |= static_cast<QueueFamilyMask>(QueueFamilyFlag::Compute);
        if (glfwGetPhysicalDevicePresentationSupport(instance, physicalDevice, i) == GLFW_TRUE)
            mask |= static_cast<QueueFamilyMask>(QueueFamilyFlag::Present);
        queueFamilyQueueTypes[i] = mask;
    }

    auto minGraphicsQueueFamily = GetNarrowestQueueFamilyIndex(queueFamilyQueueTypes | std::views::filter(isGraphicsQueue));
    auto minPresentQueueFamily  = GetNarrowestQueueFamilyIndex(queueFamilyQueueTypes | std::views::filter(isPresentQueue));
    auto minComputeQueueFamily  = GetNarrowestQueueFamilyIndex(queueFamilyQueueTypes | std::views::filter(isComputeQueue));

    // If one of the required queues is not present, reject the device
    if (minGraphicsQueueFamily.first < 0 || minPresentQueueFamily.first < 0 || minComputeQueueFamily.first < 0)
    {
        deviceScore.score = -1;
        return deviceScore;
    }
    
    // Decrease score if queue is shared by more than one queue family
    deviceScore.score += (9 - std::clamp((int)minGraphicsQueueFamily.second, 1, 3)
                            - std::clamp((int)minPresentQueueFamily.second , 1, 3)
                            - std::clamp((int)minComputeQueueFamily.second , 1, 3)) * 10;
    
    // TODO: Query for required extension support

    deviceScore.graphicsQueueIndex = minGraphicsQueueFamily.first;
    deviceScore.presentQueueIndex = minPresentQueueFamily.first;
    deviceScore.computeQueueIndex = minComputeQueueFamily.first;

    return deviceScore;
}

void blossom::Device::ChoosePhysicalDevice(vk::Instance instance, std::vector<vk::PhysicalDevice> &physicalDevices)
{
    PhysicalDeviceScore topScore, currentScore;
    vk::PhysicalDevice bestDevice;

    for (const auto &device : physicalDevices)
    {
        currentScore = GetDeviceScore(instance, device);
        if (currentScore.score > topScore.score) 
        {
            bestDevice = device;
            topScore = currentScore;
        }
    }

    m_PhysicalDevice     = bestDevice;
    m_GraphicsQueueIndex = static_cast<uint32_t>(topScore.graphicsQueueIndex);
    m_ComputeQueueIndex  = static_cast<uint32_t>(topScore.computeQueueIndex);
    m_PresentQueueIndex  = static_cast<uint32_t>(topScore.presentQueueIndex);
}

void blossom::Device::CreateLogicalDevice()
{
    std::string deviceName(m_PhysicalDevice.getProperties().deviceName);
    
    // TODO: Change prints to logging system
    std::print("Chose {} as physical device\n", deviceName);

    float priority = 0.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

    std::set<uint32_t> uniqueQueueIndices({ m_GraphicsQueueIndex, m_ComputeQueueIndex, m_PresentQueueIndex });

    for (const auto &uniqueQueue : uniqueQueueIndices)
    {
        // TODO: Support multiple queues created for the same index
        queueCreateInfos.push_back({
            .queueFamilyIndex = uniqueQueue,
            .queueCount = 1,
            .pQueuePriorities = &priority
        });
    }

    const char *deviceExtensions[] = { vk::KHRSwapchainExtensionName };

    vk::PhysicalDeviceVulkan13Features vulkan13Features;
    vulkan13Features.dynamicRendering = vk::True;
    vulkan13Features.synchronization2 = vk::True;

    vk::PhysicalDeviceFeatures2 deviceFeatures;

    vk::DeviceCreateInfo deviceCI = {
        .flags = vk::DeviceCreateFlags(),
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data()
    };

    vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features> deviceCreateChain = {
        deviceCI,
        deviceFeatures,
        vulkan13Features
    };

    vk::Result result;

    std::tie(result, m_LogicalDevice) = m_PhysicalDevice.createDevice(deviceCreateChain.get<vk::DeviceCreateInfo>());
    switch (result)
    {
        case vk::Result::eSuccess:
            break;
        case vk::Result::eErrorExtensionNotPresent:
            throw Error("A requested extension is not present!", result);
        case vk::Result::eErrorFeatureNotPresent:
            throw Error("A requested feature is not present!", result);
        case vk::Result::eErrorTooManyObjects:
            throw Error("Physical device does not support this many requested devices!", result);
        case vk::Result::eErrorDeviceLost:
            throw Error("Lost physical device!", result);
        default:
            throw blossom::Error("Unable to create logical device!", result);
    }

    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_LogicalDevice);

    // TODO: Update to support multiple queue indices on the same queue family index
    m_GraphicsQueue = m_LogicalDevice.getQueue(m_GraphicsQueueIndex, 0);
    m_PresentQueue = m_LogicalDevice.getQueue(m_GraphicsQueueIndex, 0);
    m_ComputeQueue = m_LogicalDevice.getQueue(m_GraphicsQueueIndex, 0);
}

blossom::Device::Device(const vk::Instance &instance) 
{
    vk::Result result;
    std::vector<vk::PhysicalDevice> physicalDevices;
    std::tie(result, physicalDevices) = instance.enumeratePhysicalDevices();
    if (result != vk::Result::eSuccess)
        throw blossom::Error("Unable to get physical devices!", result);

    ChoosePhysicalDevice(instance, physicalDevices);
    CreateLogicalDevice();
}
