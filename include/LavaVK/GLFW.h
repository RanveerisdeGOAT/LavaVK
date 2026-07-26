#ifndef LAVAVK_GLFW_H
#define LAVAVK_GLFW_H

#include <vector>

// Forward declaration of GLFWwindow handle to avoid including full GLFW headers in project headers
struct GLFWwindow;

namespace LavaVK::GLFW
{
    /**
     * @brief Queries and returns the required Vulkan instance extensions needed for GLFW window surface creation.
     * * Wraps `glfwGetRequiredInstanceExtensions` to retrieve platform-specific extensions
     * (e.g., `VK_KHR_surface`, `VK_KHR_win32_surface`, `VK_KHR_xcb_surface`) required to display
     * rendered frames to a GLFW window.
     * * @return std::vector<const char*> Vector of C-strings containing extension names.
     */
    [[nodiscard]] std::vector<const char*> requiredInstanceExtensions();
}

#endif // LAVAVK_GLFW_H