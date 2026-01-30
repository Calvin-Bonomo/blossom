#include "device.hpp"

using namespace blossom;

struct PhysicalDeviceScore 
{
  int score;
  int32_t graphicsQueue;
  int32_t presentQueue;
  int32_t computeQueue;
};

PhysicalDeviceScore GetDeviceScore(vk::raii::PhysicalDevice physicalDevice) 
{

}

static Device CreateDevice(vk::raii::Instance instance)
{
}