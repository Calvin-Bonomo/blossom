#include "window.hpp"

blossom::Window::Window(uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height)
{
    if (glfwInit() == GLFW_FALSE)
        throw std::runtime_error("Unable to initialize glfw!");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_GLFWWindow = glfwCreateWindow(width, height, "Blossom", nullptr, nullptr);
    if (!m_GLFWWindow)
        throw std::runtime_error("Unable to create window with glfw!");
}
