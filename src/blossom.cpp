#include "blossom.hpp"

#include <stdexcept>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

using namespace blossom;

vk::Instance Blossom::CreateInstance()
{
    vk::ApplicationInfo appInfo {
        .pApplicationName = "Blossom",
        .applicationVersion = 1,
        .pEngineName = nullptr,
        .engineVersion = 1,
        .apiVersion = vk::ApiVersion14
    };
    auto instanceLayers = GetInstanceLayers();
    auto instanceExtensions = GetInstanceExtensions();
    vk::InstanceCreateInfo instanceCI {
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(instanceLayers.size()),
        .ppEnabledLayerNames = instanceLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
        .ppEnabledExtensionNames = instanceExtensions.data()
    };

    auto res = vk::createInstance(instanceCI);
    if (res.result != vk::Result::eSuccess)
        throw std::runtime_error(vk::to_string(res.result));
    auto instance = res.value;
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
    return instance;
}


std::vector<const char *> Blossom::GetInstanceLayers()
{
    std::vector<const char *> layerNames;

    auto res = vk::enumerateInstanceLayerProperties();
    if (res.result != vk::Result::eSuccess)
        throw std::runtime_error(vk::to_string(res.result));
    auto layerProperties = res.value;

#ifndef NDEBUG
    layerNames.push_back("VK_LAYER_KHRONOS_validation");
#endif
    
    // Ensure that each instance layer name requested has an associated property
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

std::vector<const char *> Blossom::GetInstanceExtensions()
{
    uint32_t extensionCount;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    if (!glfwExtensions)
        throw std::runtime_error("Unable to get extensions required by glfw!");

    return std::vector<const char *>(glfwExtensions, glfwExtensions + extensionCount);
}
