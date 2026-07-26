#include <vector>
#include "../include/LavaVK/GLFW.h"

#include <GLFW/glfw3.h>

namespace LavaVK::GLFW
{

    std::vector<const char*> requiredInstanceExtensions()
    {
        uint32_t count = 0;

        const char** extensions =
            glfwGetRequiredInstanceExtensions(&count);

        if (!extensions)
            return {};

        return {extensions, extensions + count};
    }

}