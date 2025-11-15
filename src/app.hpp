#pragma once

import vulkan_hpp;

#include <GLFW/glfw3.h>

#include <vector>

class App {
public:
    App();
    ~App();

    void Run();
private:
    void InitGLFW();
    void InitVulkan();
    std::vector<const char *> GetInstanceLayers();
    std::vector<const char *> GetInstanceExtensions();
private:
    GLFWwindow *m_Window;
    vk::Instance m_Instance;
};

