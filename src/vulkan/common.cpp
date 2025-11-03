#include "common.hpp"
#include <stdexcept>

void vk_check(VkResult result, const std::string& msg)
{
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(msg);
    }
}

int WIDTH = 1920;
int HEIGHT = 1080;