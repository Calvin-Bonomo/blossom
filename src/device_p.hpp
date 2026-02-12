#pragma once

#include "vulkan/vulkan.hpp"

namespace blossom 
{
    struct Device
    {
        Device(const vk::Instance &instance);

        operator vk::Device() const 
        {
            return m_LogicalDevice;
        }

    private:
        struct PhysicalDeviceScore 
        {
            int score;
            int32_t graphicsQueueIndex;
            int32_t presentQueueIndex;
            int32_t computeQueueIndex;
        };

    private:
        static PhysicalDeviceScore GetDeviceScore(vk::Instance instance, vk::PhysicalDevice physicalDevice);
        void ChoosePhysicalDevice(vk::Instance instance, std::vector<vk::PhysicalDevice> &physicalDevices);
        void CreateLogicalDevice();

    private:
        vk::Device m_LogicalDevice;
        vk::PhysicalDevice m_PhysicalDevice;

        uint32_t m_GraphicsQueueIndex;
        uint32_t m_PresentQueueIndex;
        uint32_t m_ComputeQueueIndex;

        vk::Queue m_GraphicsQueue;
        vk::Queue m_PresentQueue;
        vk::Queue m_ComputeQueue;
    };
}
