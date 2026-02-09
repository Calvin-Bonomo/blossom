#pragma once

#include <GLFW/glfw3.h>

#include "vulkan/vulkan.hpp"

namespace blossom 
{
    class Window 
    {
    public:
        Window(GLFWwindow *window): m_GLFWWindow(window) { }
        ~Window();

        vk::SurfaceKHR GetSurface(vk::Instance &instance);

    private:
        static void OnResize(GLFWwindow *window, int width, int height);

    private:
        GLFWwindow *m_GLFWWindow;

        uint32_t m_Width;
        uint32_t m_Height;
    };
}
