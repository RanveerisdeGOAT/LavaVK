/**
 * @file Surface.h
 * @brief Header-only RAII wrapper for managing a Vulkan surface handle without exposing Vulkan headers in public signatures.
 */

#ifndef LAVAVK_SURFACE_H
#define LAVAVK_SURFACE_H


#include <vulkan/vulkan.h>
#include <functional>
#include <stdexcept>
#include <cstdint>
#include "Instance.hpp"

namespace LavaVK {
    /**
     * @brief Simple two-dimensional unsigned extent, mirroring `VkExtent2D`.
     *
     * @details Used throughout LavaVK (surface capabilities, swapchain
     * extent, viewport/scissor sizing) wherever a width/height pair is
     * needed without pulling in `vulkan.h`'s `VkExtent2D` directly.
     */
    struct Extent2D {
        /** @brief Width in pixels. */
        uint32_t width = 0;
        /** @brief Height in pixels. */
        uint32_t height = 0;
    };

    /**
     * @brief Abstract surface color formats matching Vulkan VkFormat equivalents.
     */
    enum class SurfaceFormat {
        Undefined = 0, /**< No format specified / format unknown. */
        B8G8R8A8_SRGB = 44, /**< Equivalent to `VK_FORMAT_B8G8R8A8_SRGB`. */
        R8G8B8A8_SRGB = 37, /**< Equivalent to `VK_FORMAT_R8G8B8A8_SRGB`. */
        B8G8R8A8_UNORM = 42, /**< Equivalent to `VK_FORMAT_B8G8R8A8_UNORM`. */
        R8G8B8A8_UNORM = 35 /**< Equivalent to `VK_FORMAT_R8G8B8A8_UNORM`. */
    };

    /**
     * @brief Abstract color space matching VkColorSpaceKHR equivalents.
     */
    enum class ColorSpace {
        SRGB_NonLinear = 0 /**< Equivalent to `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR`. */
    };

    /**
     * @brief Pair holding a surface format and color space.
     */
    struct SurfaceFormatInfo {
        /** @brief Pixel format the surface may be presented in. */
        SurfaceFormat format = SurfaceFormat::Undefined;
        /** @brief Color space associated with #format. */
        ColorSpace colorSpace = ColorSpace::SRGB_NonLinear;
    };

    /**
     * @brief Abstract presentation modes matching VkPresentModeKHR.
     */
    enum class PresentMode {
        Immediate = 0, /**< Equivalent to `VK_PRESENT_MODE_IMMEDIATE_KHR`. Images are presented immediately; no V-Sync, may cause tearing. */
        Mailbox = 1, /**< Equivalent to `VK_PRESENT_MODE_MAILBOX_KHR`. Triple-buffered V-Sync with no tearing and low latency. */
        Fifo = 2, /**< Equivalent to `VK_PRESENT_MODE_FIFO_KHR`. Guaranteed V-Sync (always supported); the classic vertical-sync queue. */
        FifoRelaxed = 3 /**< Equivalent to `VK_PRESENT_MODE_FIFO_RELAXED_KHR`. Like #Fifo but allows tearing if the application misses a frame. */
    };

    /**
     * @brief Clean C++ encapsulation of `VkSurfaceCapabilitiesKHR`.
     *
     * @details Queried from a physical device / surface pair to determine
     * valid swapchain image counts, extents, and the formats and present
     * modes the surface supports. Used by `SwapChain` when selecting
     * swapchain creation parameters.
     */
    struct SurfaceCapabilities {
        /** @brief Minimum number of images the swapchain must contain. */
        uint32_t minImageCount = 0;
        /** @brief Maximum number of images the swapchain may contain (0 means no limit). */
        uint32_t maxImageCount = 0;

        /** @brief Current extent of the surface, if fixed by the platform. */
        Extent2D currentExtent{};
        /** @brief Smallest valid swapchain image extent. */
        Extent2D minImageExtent{};
        /** @brief Largest valid swapchain image extent. */
        Extent2D maxImageExtent{};

        /** @brief The surface's current transform relative to the device's natural orientation. */
        uint32_t currentTransform = 0;
        /** @brief Bitmask of all transforms supported by the surface. */
        uint32_t supportedTransforms = 0;

        /** @brief Surface formats (pixel format + color space) supported by the surface. */
        std::vector<SurfaceFormatInfo> formats;
        /** @brief Presentation modes supported by the surface. */
        std::vector<PresentMode> presentModes;
    };

    /**
     * @brief Type alias for the surface creation function.
     * Receives the native VkInstance handle and returns a native VkSurfaceKHR handle.
     */
    using SurfaceCreator = std::function<VkSurfaceKHR(VkInstance)>;

    /**
     * @brief Manages the lifetime of a `VkSurfaceKHR` handle without exposing Vulkan handles
     * directly in its public method signatures.
     *
     * @details
     * LavaVK is windowing-library agnostic: rather than depending on GLFW,
     * SDL, or a specific platform API to create a `VkSurfaceKHR`, `Surface`
     * takes a `SurfaceCreator` callback supplied by the application, which
     * is invoked once during construction with the owning `VkInstance` and
     * is expected to return a valid `VkSurfaceKHR` (e.g. via
     * `glfwCreateWindowSurface`). The resulting handle is then owned and
     * destroyed by this class via RAII. Copying is disabled; moving
     * transfers ownership.
     *
     * Example, creating a surface from a GLFW window:
     * @code
     * LavaVK::Surface surface(instance, [window](VkInstance vkInst) -> VkSurfaceKHR {
     *     VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
     *     if (glfwCreateWindowSurface(vkInst, window, nullptr, &rawSurface) != VK_SUCCESS) {
     *         return VK_NULL_HANDLE;
     *     }
     *     return rawSurface;
     * });
     * @endcode
     */
    class Surface {
    public:
        /**
         * @brief Constructs a Surface object by invoking a surface creator delegate.
         *
         * @param instance Reference to the active LavaVK Instance the surface will belong to.
         * @param creator Callable function (e.g. GLFW, SDL, or native OS lambda) that generates the VkSurfaceKHR handle.
         * @throw std::runtime_error Thrown if @p creator is empty, or if it returns a null handle.
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
         * @brief Move constructor. Transfers ownership of the surface and its owning instance handle.
         * @param other The surface being moved from; left in an empty, destructible state.
         */
        Surface(Surface &&other) noexcept
            : m_instance(other.m_instance),
              m_surface(other.m_surface) {
            other.m_surface = VK_NULL_HANDLE;
            other.m_instance = VK_NULL_HANDLE;
        }

        /**
         * @brief Move assignment operator. Destroys any currently owned surface, then transfers
         * ownership of @p other's surface and instance handle to this object.
         * @param other The surface being moved from; left in an empty, destructible state.
         * @return Reference to this surface.
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
         * @return Native `VkSurfaceKHR` handle, or `VK_NULL_HANDLE` if moved-from.
         */
        [[nodiscard]]
        VkSurfaceKHR native() const {
            return m_surface;
        }

        /**
         * @brief Non-Vulkan handle exporter for external window/UI frameworks (e.g., ImGui).
         * @return Raw `uint64_t` representation of the underlying Vulkan surface pointer.
         */
        [[nodiscard]]
        uint64_t rawHandle() const {
            return reinterpret_cast<uint64_t>(m_surface);
        }

    private:
        /**
         * @brief Destroys the owned `VkSurfaceKHR` handle, if any, and resets it to `VK_NULL_HANDLE`.
         * @details Shared by the destructor and move-assignment operator so surface
         * teardown logic lives in exactly one place.
         */
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