import vulkan_hpp;

#include "app.hpp"

#include <exception>
#include <print>
#include <cstdlib>
#include <algorithm>
#include <cstring>

int main() {
    App app;
    app.Run();
    return 0;
}

App::App()
{
    InitGLFW();
    InitVulkan();
}

App::~App() 
{
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
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_Window = glfwCreateWindow(800, 600, "Blossom", nullptr, nullptr);
}

void App::InitVulkan()
{
    vk::ApplicationInfo appInfo("Blossom", 1, nullptr, 1, vk::ApiVersion14);
    
    try 
    {
        auto instanceLayers = GetInstanceLayers();
        auto instanceExtensions = GetInstanceExtensions();

        vk::InstanceCreateInfo instanceCI(
            {}, 
            &appInfo, 
            instanceLayers.size(), 
            instanceLayers.data(), 
            instanceExtensions.size(), 
            instanceExtensions.data());

        m_Instance = vk::createInstance(instanceCI);
    } 
    catch (std::exception const &e) 
    {
        std::print(stderr, "Unable to create Vulkan instance!\n{}", e.what());
        exit(1);
    }
}

std::vector<const char *> App::GetInstanceLayers()
{
    std::vector<vk::LayerProperties> layerProperties = vk::enumerateInstanceLayerProperties();

    std::vector<const char *> layerNames;

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
