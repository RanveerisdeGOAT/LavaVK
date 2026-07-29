#ifndef LAVAVK_SYNC_H
#define LAVAVK_SYNC_H

#include <vulkan/vulkan.h>
#include <cstdint>

#include "Instance.hpp"

namespace LavaVK {

class Device;

/**
 * @brief CPU-GPU synchronization primitive.
 *
 * A Fence is used by the CPU to determine when GPU work has completed.
 * Unlike semaphores, fences can be waited on and reset directly by the CPU.
 */
class Fence
{
public:

    /**
     * @brief Constructs an empty fence.
     */
    Fence() = default;

    /**
     * @brief Creates a Vulkan fence.
     *
     * @param device Logical device used to create the fence.
     * @param signaled Whether the fence should begin in the signaled state.
     */
    Fence(Device& device, bool signaled = false);

    /**
     * @brief Destroys the fence.
     */
    ~Fence();

    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;

    /**
     * @brief Transfers ownership from another fence.
     */
    Fence(Fence&& other) noexcept;

    /**
     * @brief Transfers ownership from another fence.
     *
     * @return Reference to this fence.
     */
    Fence& operator=(Fence&& other) noexcept;

    /**
     * @brief Waits until the fence becomes signaled.
     *
     * @param timeout Maximum wait time in nanoseconds.
     * @return Vulkan status code.
     */
    [[nodiscard]]
    Result wait(uint64_t timeout = UINT64_MAX) const;

    /**
     * @brief Resets the fence to the unsignaled state.
     *
     * @return Vulkan status code.
     */
    [[nodiscard]]
    Result reset() const;

    /**
     * @brief Returns the native Vulkan fence.
     */
    [[nodiscard]]
    VkFence native() const
    {
        return m_fence;
    }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkFence m_fence = VK_NULL_HANDLE;
};

/**
 * @brief GPU-GPU synchronization primitive.
 *
 * A Semaphore is used to synchronize operations performed by GPU queues.
 * Semaphores are commonly used when acquiring swapchain images,
 * submitting command buffers, and presenting rendered images.
 *
 * Unlike Fence, semaphores cannot be waited on or reset directly by the CPU.
 */
class Semaphore
{
public:

    /**
     * @brief Constructs an empty semaphore.
     */
    Semaphore() = default;

    /**
     * @brief Creates a Vulkan semaphore.
     *
     * @param device Logical device used to create the semaphore.
     */
    explicit Semaphore(Device& device);

    /**
     * @brief Destroys the semaphore.
     */
    ~Semaphore();

    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

    /**
     * @brief Transfers ownership from another semaphore.
     */
    Semaphore(Semaphore&& other) noexcept;

    /**
     * @brief Transfers ownership from another semaphore.
     *
     * @return Reference to this semaphore.
     */
    Semaphore& operator=(Semaphore&& other) noexcept;

    /**
     * @brief Returns the native Vulkan semaphore.
     */
    [[nodiscard]]
    VkSemaphore native() const
    {
        return m_semaphore;
    }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkSemaphore m_semaphore = VK_NULL_HANDLE;
};

} // namespace LavaVK

#endif // LAVAVK_SYNC_H