#ifndef LAVAVK_FENCE_H
#define LAVAVK_FENCE_H

#include <vulkan/vulkan.h>
#include <cstdint>

namespace LavaVK {
    class Device;

    class Fence {
    public:
        Fence() = default;

        /**
         * @brief Creates a Vulkan Fence wrapper.
         * @param device The LavaVK Device instance.
         * @param signaled Whether to create the fence in a signaled state (default: false).
         */
        Fence(Device &device, bool signaled = false);

        ~Fence();

        // Prevent copying
        Fence(const Fence &) = delete;

        Fence &operator=(const Fence &) = delete;

        // Allow moving
        Fence(Fence &&other) noexcept;

        Fence &operator=(Fence &&other) noexcept;

        /**
         * @brief Waits for the fence to reach the signaled state.
         * @param timeout Timeout in nanoseconds (default: UINT64_MAX).
         * @return VkResult status code.
         */
        VkResult wait(uint64_t timeout = UINT64_MAX) const;

        /**
         * @brief Resets the fence to an unsignaled state.
         * @return VkResult status code.
         */
        VkResult reset() const;

        /**
         * @brief Returns the underlying VkFence handle.
         */
        [[nodiscard]] VkFence native() const { return m_fence; }

    private:
        VkDevice m_device = VK_NULL_HANDLE;
        VkFence m_fence = VK_NULL_HANDLE;
    };
}

#endif // LAVAVK_FENCE_H
