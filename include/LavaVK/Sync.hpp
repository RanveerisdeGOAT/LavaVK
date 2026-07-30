#ifndef LAVAVK_SYNC_H
#define LAVAVK_SYNC_H

#include <vulkan/vulkan.h>
#include <cstdint>

#include "Core.hpp"
#include "Instance.hpp"


namespace LavaVK {
    class Device;
    /**
     * @brief CPU-GPU synchronization primitive.
     *
     * @details
     * A Fence is used by the CPU to determine when GPU work has completed.
     * Unlike semaphores, fences can be waited on and reset directly by the
     * CPU. A common use is to gate CPU access to a resource (such as
     * re-recording a per-frame command buffer) until the GPU has finished
     * consuming it. `Fence` is move-only RAII: the underlying `VkFence` is
     * created in the constructor and destroyed in the destructor.
     *
     * Example, the classic "wait for the previous frame, then reset" pattern:
     * @code
     * LavaVK::Fence inFlightFence(device, /*signaled=*\/true);
     *
     * // Each frame:
     * inFlightFence.wait();
     * inFlightFence.reset();
     * // ... record and submit commands, signaling inFlightFence on completion ...
     * @endcode
     */
    class Fence {
    public:
        /**
         * @brief Constructs an empty fence.
         * @details Leaves the fence in an unowned state (`native()` returns
         * `VK_NULL_HANDLE`) until move-assigned from a constructed `Fence`.
         */
        Fence() = default;

        /**
         * @brief Creates a Vulkan fence.
         *
         * @param device Logical device used to create the fence.
         * @param signaled Whether the fence should begin in the signaled state.
         * Pass `true` for fences awaited before any work has been submitted
         * (e.g. the first frame's in-flight fence), so the initial wait does
         * not block forever.
         * @throw std::runtime_error If `vkCreateFence` fails.
         */
        Fence(Device &device, bool signaled = false);

        /**
         * @brief Destroys the fence.
         * @details Calls `vkDestroyFence` on the owned handle if it is not
         * `VK_NULL_HANDLE`; a moved-from `Fence` is a no-op.
         */
        ~Fence();

        Fence(const Fence &) = delete;

        Fence &operator=(const Fence &) = delete;

        /**
         * @brief Transfers ownership from another fence.
         * @param other The fence being moved from; left in an empty, destructible state.
         */
        Fence(Fence &&other) noexcept;

        /**
         * @brief Transfers ownership from another fence.
         *
         * @param other The fence being moved from; left in an empty, destructible state.
         * @return Reference to this fence.
         */
        Fence &operator=(Fence &&other) noexcept;

        /**
         * @brief Waits until the fence becomes signaled.
         *
         * @param timeout Maximum wait time in nanoseconds. Defaults to
         * `UINT64_MAX`, which blocks indefinitely until the fence signals.
         * @return #Result wrapping the Vulkan status code returned by
         * `vkWaitForFences` (e.g. success, or a timeout status).
         */
        Result wait(uint64_t timeout = UINT64_MAX) const;

        /**
         * @brief Resets the fence to the unsignaled state.
         *
         * @return #Result wrapping the Vulkan status code returned by `vkResetFences`.
         */
        Result reset() const;

        /**
         * @brief Returns the native Vulkan fence.
         * @return The underlying `VkFence` handle, or `VK_NULL_HANDLE` if unconstructed/moved-from.
         */
        [[nodiscard]]
        VkFence native() const {
            return m_fence;
        }

    private:
        VkDevice m_device = VK_NULL_HANDLE;
        VkFence m_fence = VK_NULL_HANDLE;
    };

    /**
     * @brief GPU-GPU synchronization primitive.
     *
     * @details
     * A Semaphore is used to synchronize operations performed by GPU
     * queues. Semaphores are commonly used when acquiring swapchain
     * images, submitting command buffers, and presenting rendered images
     * — e.g. an "image available" semaphore that a submit waits on before
     * writing to a swapchain image, and a "render finished" semaphore that
     * a submit signals and that the present call waits on.
     *
     * Unlike Fence, semaphores cannot be waited on or reset directly by
     * the CPU; they are only ever waited on / signaled by GPU queue
     * operations. `Semaphore` is move-only RAII: the underlying
     * `VkSemaphore` is created in the constructor and destroyed in the
     * destructor.
     *
     * Example:
     * @code
     * LavaVK::Semaphore imageAvailable(device);
     * LavaVK::Semaphore renderFinished(device);
     *
     * swapchain.acquireImage(imageIndex); // signals imageAvailable internally
     * device.submit(LavaVK::QueueType::GRAPHICS, frameIndex,
     *               { imageAvailable.native() }, { ... },
     *               { renderFinished.native() }, &inFlightFence);
     * @endcode
     */
    class Semaphore {
    public:
        /**
         * @brief Constructs an empty semaphore.
         * @details Leaves the semaphore in an unowned state (`native()`
         * returns `VK_NULL_HANDLE`) until move-assigned from a constructed
         * `Semaphore`.
         */
        Semaphore() = default;

        /**
         * @brief Creates a Vulkan semaphore.
         *
         * @param device Logical device used to create the semaphore.
         * @throw std::runtime_error If `vkCreateSemaphore` fails.
         */
        explicit Semaphore(Device &device);

        /**
         * @brief Destroys the semaphore.
         * @details Calls `vkDestroySemaphore` on the owned handle if it is
         * not `VK_NULL_HANDLE`; a moved-from `Semaphore` is a no-op.
         */
        ~Semaphore();

        Semaphore(const Semaphore &) = delete;

        Semaphore &operator=(const Semaphore &) = delete;

        /**
         * @brief Transfers ownership from another semaphore.
         * @param other The semaphore being moved from; left in an empty, destructible state.
         */
        Semaphore(Semaphore &&other) noexcept;

        /**
         * @brief Transfers ownership from another semaphore.
         *
         * @param other The semaphore being moved from; left in an empty, destructible state.
         * @return Reference to this semaphore.
         */
        Semaphore &operator=(Semaphore &&other) noexcept;

        /**
         * @brief Returns the native Vulkan semaphore.
         * @return The underlying `VkSemaphore` handle, or `VK_NULL_HANDLE` if unconstructed/moved-from.
         */
        [[nodiscard]]
        VkSemaphore native() const {
            return m_semaphore;
        }

    private:
        VkDevice m_device = VK_NULL_HANDLE;
        VkSemaphore m_semaphore = VK_NULL_HANDLE;
    };
} // namespace LavaVK

#endif // LAVAVK_SYNC_H