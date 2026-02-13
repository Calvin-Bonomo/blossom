#include "blossom.hpp"

#include <stdexcept>
#include <tuple>

#include "error_p.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

int main()
{
    blossom::Blossom enigne;
}

blossom::Blossom::~Blossom()
{
    if (m_Instance != nullptr)
        m_Instance.destroy();
}

// Instance creation stuff

vk::Instance blossom::Blossom::CreateInstance()
{
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

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

    vk::Result result;
    vk::Instance instance;
    std::tie(result, instance) = vk::createInstance(instanceCI);

    if (result != vk::Result::eSuccess)
        throw blossom::Error("Unable to create Vulkan instance!", result);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
    return instance;
}


std::vector<const char *> blossom::Blossom::GetInstanceLayers()
{
    std::vector<const char *> layerNames;
    vk::Result result;
    std::vector<vk::LayerProperties> layerProperties;
    std::tie(result, layerProperties) = vk::enumerateInstanceLayerProperties();
    if (result != vk::Result::eSuccess)
        throw blossom::Error("Unable to enumerate instance layer properties!", result);

#ifndef NDEBUG
    layerNames.push_back("VK_LAYER_KHRONOS_validation");
#endif
    
    // Ensure that each instance layer requested has an associated property
    if (!std::all_of(layerNames.begin(), layerNames.end(), [&layerProperties](const char *name)
            {
                return std::any_of(
                        layerProperties.begin(), 
                        layerProperties.end(), 
                        [&name](vk::LayerProperties const &property) { return strcmp(name, property.layerName) == 0; });
            }))
    {
        throw std::runtime_error("Unable to get requested instance layers!");
    }

    return layerNames;
}

std::vector<const char *> blossom::Blossom::GetInstanceExtensions()
{
    uint32_t extensionCount;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    if (!glfwExtensions)
        throw std::runtime_error("Unable to get extensions required by glfw!");

    return std::vector<const char *>(glfwExtensions, glfwExtensions + extensionCount);
}

