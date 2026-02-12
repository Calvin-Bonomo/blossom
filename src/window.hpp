#pragma once

#include <GLFW/glfw3.h>

namespace blossom 
{
    class Window 
    {
    public:
        Window(uint32_t width, uint32_t height);
        ~Window();

    private:
        static void OnResize(GLFWwindow *window, int width, int height);

    private:
        GLFWwindow *m_GLFWWindow;

        uint32_t m_Width;
        uint32_t m_Height;
    };
}
