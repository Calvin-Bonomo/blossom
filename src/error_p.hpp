#pragma once

#include <exception>
#include <string>

#include "vulkan/vulkan.hpp"

namespace blossom 
{
    class Error : public std::exception
    {
    public:
        Error(const std::string &engineMsg, vk::Result error);
        const char *what() const noexcept;
    private:
        std::string m_Msg;
    };
}
