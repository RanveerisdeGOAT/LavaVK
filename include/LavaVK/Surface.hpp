/**
 * @file Surface.h
 * @brief Header-only RAII wrapper for managing a Vulkan surface handle without exposing Vulkan headers in public signatures.
 */

#ifndef LAVAVK_SURFACE_H
#define LAVAVK_SURFACE_H

#include "Instance.hpp"

#include <vulkan/vulkan.h>
#include <functional>
#include <stdexcept>
#include <cstdint>

namespace LavaVK {
    struct Extent2D {
        uint32_t width = 0;
        uint32_t height = 0;
    };

    /**
     * @brief Abstract surface color formats matching Vulkan VkFormat equivalents.
     */
    enum class SurfaceFormat {
        Undefined = 0,
        B8G8R8A8_SRGB = 44, // VK_FORMAT_B8G8R8A8_SRGB
        R8G8B8A8_SRGB = 37, // VK_FORMAT_R8G8B8A8_SRGB
        B8G8R8A8_UNORM = 42, // VK_FORMAT_B8G8R8A8_UNORM
        R8G8B8A8_UNORM = 35 // VK_FORMAT_R8G8B8A8_UNORM
    };

    /**
     * @brief Abstract color space matching VkColorSpaceKHR equivalents.
     */
    enum class ColorSpace {
        SRGB_NonLinear = 0 // VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
    };

    /**
     * @brief Pair holding a surface format and color space.
     */
    struct SurfaceFormatInfo {
        SurfaceFormat format = SurfaceFormat::Undefined;
        ColorSpace colorSpace = ColorSpace::SRGB_NonLinear;
    };

    /**
     * @brief Abstract presentation modes matching VkPresentModeKHR.
     */
    enum class PresentMode {
        Immediate = 0, // VK_PRESENT_MODE_IMMEDIATE_KHR (No V-Sync)
        Mailbox = 1, // VK_PRESENT_MODE_MAILBOX_KHR (Triple buffering V-Sync)
        Fifo = 2, // VK_PRESENT_MODE_FIFO_KHR (Guaranteed V-Sync)
        FifoRelaxed = 3 // VK_PRESENT_MODE_FIFO_RELAXED_KHR
    };

    /**
     * @brief Clean C++ encapsulation of VkSurfaceCapabilitiesKHR.
     */
    struct SurfaceCapabilities {
        uint32_t minImageCount = 0;
        uint32_t maxImageCount = 0;

        Extent2D currentExtent{};
        Extent2D minImageExtent{};
        Extent2D maxImageExtent{};

        uint32_t currentTransform = 0;
        uint32_t supportedTransforms = 0;

        std::vector<SurfaceFormatInfo> formats;
        std::vector<PresentMode> presentModes;
    };

    /**
     * @brief Type alias for the surface creation function.
     * Receives the native VkInstance handle and returns a native VkSurfaceKHR handle.
     */
    using SurfaceCreator = std::function<VkSurfaceKHR(VkInstance)>;

    /**
     * @brief Manages the lifetime of a VkSurfaceKHR handle without exposing Vulkan handles
     * directly in its public method signatures.
     */
    class Surface {
    public:
        /**
         * @brief Constructs a Surface object by invoking a surface creator delegate.
         * * @param instance Reference to the active LavaVK Instance.
         * @param creator Callable function (e.g. GLFW, SDL, or native OS lambda) that generates the VkSurfaceKHR handle.
         * @throw std::runtime_error Thrown if the creator returns a null handle.
         */
        explicit Surface(const Instance &instance, const SurfaceCreator &creator)
            : m_instance(instance.native()) {
            if (!creator) {
                throw std::runtime_error("[LavaVK ERROR] Surface creator function is invalid.");
            }

            m_surface = creator(m_instance);

            if (m_surface == VK_NULL_HANDLE) {
                throw std::runtime_error("[LavaVK ERROR] Failed to create window surface handle.");
            }
        }

        /**
         * @brief Destructor. Destroys the managed VkSurfaceKHR handle if valid.
         */
        ~Surface() {
            cleanup();
        }

        /// @name Deleted Copy Operations
        /// @{
        Surface(const Surface &) = delete;

        Surface &operator=(const Surface &) = delete;

        /// @}

        /**
         * @brief Move constructor.
         */
        Surface(Surface &&other) noexcept
            : m_instance(other.m_instance),
              m_surface(other.m_surface) {
            other.m_surface = VK_NULL_HANDLE;
            other.m_instance = VK_NULL_HANDLE;
        }

        /**
         * @brief Move assignment operator.
         */
        Surface &operator=(Surface &&other) noexcept {
            if (this != &other) {
                cleanup();

                m_instance = other.m_instance;
                m_surface = other.m_surface;

                other.m_surface = VK_NULL_HANDLE;
                other.m_instance = VK_NULL_HANDLE;
            }
            return *this;
        }

        /**
         * @brief Internal getter for LavaVK engine components (e.g., Device, Swapchain).
         * * @return Native VkSurfaceKHR handle.
         */
        [[nodiscard]]
        VkSurfaceKHR native() const {
            return m_surface;
        }

        /**
         * @brief Non-Vulkan handle exporter for external window/UI frameworks (e.g., ImGui).
         * * @return Raw uint64_t representation of the underlying Vulkan surface pointer.
         */
        [[nodiscard]]
        uint64_t rawHandle() const {
            return reinterpret_cast<uint64_t>(m_surface);
        }

    private:
        void cleanup() {
            if (m_surface != VK_NULL_HANDLE && m_instance != VK_NULL_HANDLE) {
                vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
                m_surface = VK_NULL_HANDLE;
            }
        }

        VkInstance m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    };
} // namespace LavaVK

#endif // LAVAVK_SURFACE_H
