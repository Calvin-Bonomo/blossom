#pragma once

#include "vulkan/vulkan.hpp"

#include "device_p.hpp"

#include "window.hpp"
#include "swapchain_p.hpp"

namespace blossom 
{
    class Blossom 
    {
    public:
        Blossom(int width = 600, int height = 400):
            m_Window(width, height),
            m_Instance(CreateInstance()),
            m_Device(m_Instance),
            m_Swapchain() { }

        // These are deleted because they have weird implications
        // for the dynamic loader. If I ever write my own loader,
        // then these may come back... at least the move semantics might.
        Blossom(const Blossom &) = delete;
        Blossom &operator=(const Blossom &) = delete;

        Blossom(Blossom &&) = delete;
        Blossom &operator=(Blossom &&) = delete;

        ~Blossom();

    private:
        vk::Instance CreateInstance();

        static std::vector<const char *> GetInstanceLayers();
        static std::vector<const char *> GetInstanceExtensions();

    private:
        Window m_Window;
        vk::Instance m_Instance;
        Device m_Device;
        Swapchain m_Swapchain;
    };
}
