#pragma once

#include "vulkan/vulkan.hpp"

#include "window.hpp"
#include "device.hpp"
#include "swapchain.hpp"

namespace blossom 
{
    class Blossom 
    {
    public:
        Blossom(int width = 600, int height = 400):
            m_Window(CreateWindow(width, height)),
            m_Instance(std::move(CreateInstance())),
            m_Device(CreateDevice()),
            m_Swapchain(CreateSwapchain()) { }

        ~Blossom();

    private:
        Window CreateWindow(int width, int height);
        vk::Instance CreateInstance();
        Device CreateDevice();
        Swapchain CreateSwapchain();

        static std::vector<const char *> GetInstanceLayers();
        static std::vector<const char *> GetInstanceExtensions();

    private:
        Window m_Window;
        vk::Instance m_Instance;
        Device m_Device;
        Swapchain m_Swapchain;
    };
}
