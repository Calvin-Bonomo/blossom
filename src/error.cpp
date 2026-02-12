#include "error_p.hpp"

#include <format>

blossom::Error::Error(const std::string &engineMsg, vk::Result error)
    : m_Msg(std::format("Blossom: {}\nVulkan: {}", engineMsg, vk::to_string(error))) { }

const char *blossom::Error::what() const noexcept
{
    return m_Msg.c_str();
}
