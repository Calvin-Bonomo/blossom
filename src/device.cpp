#include "device.hpp"

using namespace blossom;

struct PhysicalDeviceScore 
{
  int score;
  int32_t graphicsQueue;
  int32_t presentQueue;
  int32_t computeQueue;
};

PhysicalDeviceScore GetDeviceScore(vk::PhysicalDevice physicalDevice) 
{

}

static Device CreateDevice(vk::Instance instance)
{
}
