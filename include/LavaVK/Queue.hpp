#ifndef LAVAVK_QUEUE_H
#define LAVAVK_QUEUE_H

#include <vulkan/vulkan.h>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace LavaVK {
    class Fence;

    class Device;

    /**
     * @brief Supported hardware queue operation types matching Vulkan queue flags.
     */
    enum class QueueType : uint32_t {
        GRAPHICS = VK_QUEUE_GRAPHICS_BIT, /**< Supports graphics rendering pipeline operations (0x00000001). */
        COMPUTE = VK_QUEUE_COMPUTE_BIT, /**< Supports compute execution pipelines (0x00000002). */
        TRANSFER = VK_QUEUE_TRANSFER_BIT, /**< Supports buffer/image memory transfer operations (0x00000004). */
        SPARSE = VK_QUEUE_SPARSE_BINDING_BIT, /**< Supports sparse resource binding (0x00000008). */

        // Custom bit flag for Presentation (avoiding Vulkan's bit flags)
        PRESENT = 0x00000020 /**< Supports surface presentation operations. */
    };

    /**
     * @brief Wrapper around VkSubmitInfo to simplify building queue submission payloads.
     * * Manages internal lifetime for wait semaphores, signal semaphores, pipeline stage masks,
     * and command buffers to prevent pointer dangling during submission.
     */
    class SubmitInfo {
    public:
        /**
         * @brief Default constructor. Initializes internal VkSubmitInfo struct fields to default values.
         */
        SubmitInfo() {
            m_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            m_info.pNext = nullptr;
            m_info.waitSemaphoreCount = 0;
            m_info.pWaitSemaphores = nullptr;
            m_info.pWaitDstStageMask = nullptr;
            m_info.commandBufferCount = 0;
            m_info.pCommandBuffers = nullptr;
            m_info.signalSemaphoreCount = 0;
            m_info.pSignalSemaphores = nullptr;
        }

        /**
         * @brief Configures semaphores that must be signaled before commands can execute.
         * @param semaphores List of Vulkan semaphores to wait on.
         * @param stages Pipeline stages at which each corresponding wait semaphore will occur.
         * @return Reference to this SubmitInfo instance for method chaining.
         */
        SubmitInfo &setWaitSemaphores(const std::vector<VkSemaphore> &semaphores,
                                      const std::vector<VkPipelineStageFlags> &stages) {
            m_waitSemaphores = semaphores;
            m_waitDstStageMasks = stages;

            m_info.waitSemaphoreCount = static_cast<uint32_t>(m_waitSemaphores.size());
            m_info.pWaitSemaphores = m_waitSemaphores.data();
            m_info.pWaitDstStageMask = m_waitDstStageMasks.data();
            return *this;
        }

        /**
         * @brief Configures command buffers to be executed by the queue.
         * @param commandBuffers Vector of Vulkan command buffers to submit.
         * @return Reference to this SubmitInfo instance for method chaining.
         */
        SubmitInfo &setCommandBuffers(const std::vector<VkCommandBuffer> &commandBuffers) {
            m_commandBuffers = commandBuffers;

            m_info.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
            m_info.pCommandBuffers = m_commandBuffers.data();
            return *this;
        }

        /**
         * @brief Configures semaphores that will be signaled once submitted command execution completes.
         * @param semaphores Vector of Vulkan semaphores to signal upon completion.
         * @return Reference to this SubmitInfo instance for method chaining.
         */
        SubmitInfo &setSignalSemaphores(const std::vector<VkSemaphore> &semaphores) {
            m_signalSemaphores = semaphores;

            m_info.signalSemaphoreCount = static_cast<uint32_t>(m_signalSemaphores.size());
            m_info.pSignalSemaphores = m_signalSemaphores.data();
            return *this;
        }

        /**
         * @brief Gets the native Vulkan VkSubmitInfo structure.
         * @return Const reference to the configured underlying VkSubmitInfo object.
         */
        [[nodiscard]] const VkSubmitInfo &native() const { return m_info; }

    private:
        VkSubmitInfo m_info{};
        std::vector<VkSemaphore> m_waitSemaphores;
        std::vector<VkPipelineStageFlags> m_waitDstStageMasks;
        std::vector<VkCommandBuffer> m_commandBuffers;
        std::vector<VkSemaphore> m_signalSemaphores;
    };

    /**
     * @brief Wrapper class managing execution operations on a Vulkan device queue (`VkQueue`).
     */
    class Queue {
    public:
        /**
         * @brief Default constructor. Constructs an uninitialized Queue handle.
         */
        Queue() = default;

        /**
         * @brief Constructs a LavaVK Queue wrapper handle from a Device and family index.
         * @param device Reference to the parent LavaVK Device instance.
         * @param family Queue family index corresponding to this queue handle.
         */
        Queue(
            Device &device,
            uint32_t family
        );

        /**
         * @brief Submits work payloads to this queue for execution on the GPU.
         * @param submitInfo Structured payload containing command buffers and synchronization primitives.
         * @param fence Fence object signaled when all submitted command buffers finish execution.
         */
        void submit(
            const SubmitInfo &submitInfo,
            Fence &fence
        ) const;

        /**
         * @brief Blocks host CPU execution until all submitted commands on this queue finish.
         */
        void waitIdle() const;

        /**
         * @brief Gets the native Vulkan queue handle (`VkQueue`).
         * @return The underlying `VkQueue` handle.
         */
        [[nodiscard]] VkQueue native() const {
            return m_queue;
        }

        /**
         * @brief Gets the queue family index associated with this queue.
         * @return The queue family index as an unsigned integer.
         */
        [[nodiscard]] uint32_t family() const {
            return m_family;
        }

    private:
        VkQueue m_queue = VK_NULL_HANDLE; /**< Native Vulkan queue handle. */
        uint32_t m_family = 0; /**< Queue family index. */
    };
} // namespace LavaVK

#endif // LAVAVK_QUEUE_H
