#ifndef EXAMPLE_GLFW_H
#define EXAMPLE_GLFW_H

#include <vector>

struct GLFWwindow;

namespace LavaVK::GLFW
{
    std::vector<const char*> requiredInstanceExtensions();
}

#endif //EXAMPLE_GLFW_H