#ifndef LAVAVK_INSTANCE_H
#define LAVAVK_INSTANCE_H

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <vector>

namespace LavaVK {
    /**
     * @brief Configuration parameters used to initialize a LavaVK Instance.
     */
    struct InstanceCreateInfo {
        /** @brief The name of the application. */
        std::string applicationName = "Application";

        /** @brief The version of the application encoded via VK_MAKE_VERSION. */
        uint32_t applicationVersion = VK_MAKE_VERSION(1, 0, 1);

        /** @brief Enables Khronos Vulkan validation layers in non-release builds (`#ifndef NDEBUG`). */
        bool enableValidation = true;

        /** @brief List of required Vulkan instance extensions. */
        std::vector<const char *> extensions;
    };

    /**
     * @brief Encapsulates a Vulkan instance (`VkInstance`) handle, managing its creation, lifetime, and cleanup.
     *
     * @details
     * `Instance` is the entry point of any LavaVK application and the first
     * object created before anything else (surfaces, GPU selection,
     * devices, etc.), since those all require a valid `VkInstance` to be
     * queried or constructed. It owns the underlying `VkInstance` handle,
     * creating it in the constructor from an `InstanceCreateInfo` and
     * destroying it in the destructor. In debug builds, validation layers
     * are enabled automatically unless disabled via
     * `InstanceCreateInfo::enableValidation`.
     *
     * This class implements RAII semantics and move operations. Copying is
     * explicitly disabled to prevent multiple deletions of the underlying
     * handle.
     *
     * Example usage, requesting the extensions GLFW needs to present to a window:
     * @code
     * uint32_t extensionCount = 0;
     * const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
     *
     * LavaVK::Instance instance({
     *     .applicationName = "My Application",
     *     .extensions = std::vector<const char *>(glfwExtensions, glfwExtensions + extensionCount)
     * });
     * @endcode
     */
    class Instance {
    public:
        /**
         * @brief Constructs a new LavaVK Instance and initializes the underlying Vulkan context.
         * * @param info Configuration structure specifying application metadata, validation rules, and extensions.
         * @throw std::runtime_error Thrown if `vkCreateInstance` fails to initialize the Vulkan context.
         */
        explicit Instance(const InstanceCreateInfo &info = {});

        /**
         * @brief Destructor. Destroys the managed `VkInstance` handle if valid.
         *
         * @details Calls `vkDestroyInstance` on the owned handle if it is
         * not `VK_NULL_HANDLE`; a moved-from `Instance` is a no-op.
         */
        ~Instance();

        /// @name Deleted Copy Operations
        /// @{
        Instance(const Instance &) = delete;

        Instance &operator=(const Instance &) = delete;

        /// @}

        /**
         * @brief Move constructor. Transfers ownership of the Vulkan instance handle from another `Instance`.
         * @param other The instance being moved from.
         */
        Instance(Instance &&other) noexcept;

        /**
         * @brief Move assignment operator. Destroys the current managed handle and acquires the handle from another `Instance`.
         * @param other The instance being moved from.
         * @return Reference to this instance.
         */
        Instance &operator=(Instance &&other) noexcept;

        /**
         * @brief Retrieves the raw, underlying native `VkInstance` handle.
         * @return The underlying `VkInstance` handle, or `VK_NULL_HANDLE` if invalid.
         */
        [[nodiscard]]
        VkInstance native() const {
            return m_instance;
        }

    private:
        /** @brief Native Vulkan instance handle. */
        VkInstance m_instance = VK_NULL_HANDLE;
    };
} // namespace LavaVK

#endif // LAVAVK_INSTANCE_H