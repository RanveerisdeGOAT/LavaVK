#ifndef LAVAVK_DEVICE_H
#define LAVAVK_DEVICE_H

#include <vulkan/vulkan.h>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <vector>
#include <utility>
#include <set>

#include "Surface.hpp"

namespace LavaVK {
    class Fence;
    class Semaphore;
    class CommandBuffer;
    class CommandPool;
    class Queue;
    class Surface;
    enum class QueueType : uint32_t;
    class Instance;
    enum class PipelineStage : VkPipelineStageFlags;
}

namespace LavaVK {
    /**
     * @brief Broad hardware category of a physical GPU, mirroring `VkPhysicalDeviceType`.
     * @details Used by #GPUHardware::type() and by #GPUHardware::selectOptimalGPU()
     * when ranking available GPUs (discrete GPUs are generally preferred).
     */
    enum class GPUType {
        Other, /**< Equivalent to `VK_PHYSICAL_DEVICE_TYPE_OTHER`. */
        Integrated, /**< Equivalent to `VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU`. */
        Discrete, /**< Equivalent to `VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU`. */
        Virtual, /**< Equivalent to `VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU`. */
        CPU /**< Equivalent to `VK_PHYSICAL_DEVICE_TYPE_CPU`. */
    };

    /**
     * @brief Contains available features of the GPU.
     */
    struct GPUFeatures {
        bool descriptorIndexing = false;
        bool runtimeDescriptorArray = false;
        bool partiallyBound = false;
        bool variableDescriptorCount = false;
        bool nonUniformIndexing = false;
        bool updateAfterBind = false;
    };

    /**
     * @brief Lightweight, queryable handle to a physical Vulkan GPU (`VkPhysicalDevice`).
     *
     * @details
     * `GPUHardware` does not own or create any Vulkan resources — it simply
     * wraps a `VkPhysicalDevice` (enumerated from an `Instance`) along with
     * its cached `VkPhysicalDeviceProperties`, and exposes convenience
     * queries used when choosing which GPU to render on and which queue
     * families to request from it. A `GPUHardware` is passed to the
     * `Device` constructor to create the logical device.
     *
     * Example, letting LavaVK pick the best available GPU automatically:
     * @code
     * const LavaVK::GPUHardware &gpu = LavaVK::GPUHardware::selectOptimalGPU(instance, surface);
     * LavaVK::Device device(gpu, { LavaVK::QueueType::GRAPHICS, LavaVK::QueueType::PRESENT }, &surface);
     * @endcode
     *
     * Or enumerating and inspecting GPUs manually:
     * @code
     * for (const LavaVK::GPUHardware &gpu : LavaVK::GPUHardware::enumerate(instance)) {
     *     std::cout << gpu.name() << " is a " << (int)gpu.type() << " device\n";
     * }
     * @endcode
     */
    class GPUHardware {
    public:
        /**
         * @brief Constructs an empty, unbound GPU handle.
         * @details `native()` returns `VK_NULL_HANDLE` until assigned from
         * a handle returned by #enumerate() or #selectOptimalGPU().
         */
        GPUHardware() = default;

        /**
         * @brief Wraps an existing physical device handle.
         * @param device The native `VkPhysicalDevice` to wrap. Its
         * properties are queried immediately via `vkGetPhysicalDeviceProperties`.
         */
        explicit GPUHardware(VkPhysicalDevice device);

        /**
         * @brief Returns every Vulkan compatible GPU in the system.
         * @param instance The LavaVK Instance to enumerate physical devices from.
         * @return A vector of #GPUHardware handles, one per `VkPhysicalDevice`
         * reported by `vkEnumeratePhysicalDevices`.
         * @throw std::runtime_error If no Vulkan-capable physical devices are found.
         */
        static std::vector<GPUHardware> enumerate(const Instance &instance);

        /**
         * @brief Returns the GPU name.
         * @return The human-readable device name reported by the Vulkan driver
         * (`VkPhysicalDeviceProperties::deviceName`).
         */
        [[nodiscard]] const std::string &name() const;

        /**
         * @brief Returns the GPU type.
         * @return The #GPUType classification of this physical device (e.g. Discrete, Integrated).
         */
        [[nodiscard]] GPUType type() const;

        /**
         * @brief Returns the Vulkan API version.
         * @return The highest Vulkan API version this device supports, encoded via `VK_MAKE_VERSION`.
         */
        [[nodiscard]] uint32_t apiVersion() const;

        /**
         * @brief Returns the Vulkan driver version.
         * @return The vendor-specific driver version, in a vendor-defined encoding.
         */
        [[nodiscard]] uint32_t driverVersion() const;

        /**
         * @brief Returns the Vulkan vendor ID.
         * @return The PCI vendor ID identifying the GPU's manufacturer.
         */
        [[nodiscard]] uint32_t vendorID() const;

        /**
         * @brief Returns the Vulkan device ID.
         * @return The PCI device ID identifying this specific GPU model.
         */
        [[nodiscard]] uint32_t deviceID() const;

        /**
         * @brief Finds a queue family supporting the requested operations.
         * @details For `QueueType::PRESENT`, this delegates to #findPresentQueueFamily().
         * For all other queue types, it searches the device's reported queue
         * families for one whose flags include the corresponding Vulkan queue bit.
         * @param type The kind of queue operations required (graphics, compute, transfer, present, ...).
         * @param surface Surface to test presentation support against; required
         * (non-null) when @p type is `QueueType::PRESENT`, otherwise ignored.
         * @return Index of a queue family satisfying @p type.
         * @throw std::runtime_error If no queue family supports the requested operations.
         */
        [[nodiscard]] uint32_t findQueueFamily(QueueType type, const Surface *surface) const;

        /**
         * @brief Returns the underlying Vulkan handle.
         * @return The native `VkPhysicalDevice` handle, or `VK_NULL_HANDLE` if unbound.
         */
        [[nodiscard]] VkPhysicalDevice native() const;

        /**
         * @brief Finds a queue family supporting surface presentation.
         * @param surface The surface to present to.
         * @return Queue family index supporting presentation.
         * @throw std::runtime_error if no queue family supports presentation.
         */
        [[nodiscard]] uint32_t findPresentQueueFamily(const Surface &surface) const;

        /**
         * @brief Checks if a specific queue family supports presenting to a given surface.
         * @param queueFamilyIndex Index of the queue family to test.
         * @param surface The surface to test presentation support against.
         * @return `true` if the queue family at @p queueFamilyIndex can present to @p surface; `false` otherwise.
         */
        [[nodiscard]] bool supportsPresentation(uint32_t queueFamilyIndex, const Surface &surface) const;

        /**
         * @brief Checks if the physical GPU supports swapchain rendering on a given surface.
         * @param surface The surface to test.
         * @return `true` if this GPU has at least one queue family that can present to
         * @p surface and reports non-empty surface formats/present modes; `false` otherwise.
         */
        [[nodiscard]] bool isSurfaceSupported(const Surface &surface) const;

        /**
         * @brief Selects the most suitable GPU available for rendering to the given surface.
         * @details Enumerates all physical devices via #enumerate() and picks
         * the best candidate that supports @p surface, generally preferring
         * discrete GPUs over integrated ones.
         * @param instance The LavaVK Instance to enumerate physical devices from.
         * @param surface The surface the selected GPU must be able to present to.
         * @return The chosen #GPUHardware.
         * @throw std::runtime_error If no suitable GPU is found.
         */
        [[nodiscard]] static GPUHardware selectOptimalGPU(const Instance &instance, const Surface &surface);

        /**
         * @brief Queries this GPU's capabilities with respect to a given surface.
         * @param surface The surface to query capabilities for.
         * @return A #SurfaceCapabilities structure describing valid image counts,
         * extents, supported formats, and supported present modes.
         */
        [[nodiscard]] SurfaceCapabilities getSurfaceCapabilities(const Surface &surface) const;

        /**
         * @brief Returns features
         *
         */
        [[nodiscard]] GPUFeatures features() const { return m_features; }

    private:
        friend class Device;

        VkPhysicalDevice m_device = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties m_properties{};
        GPUFeatures m_features;
    };

    /**
     * @brief Manages the Vulkan physical device selection, logical device instantiation (`VkDevice`),
     * and graphics queue retrieval.
     *
     * @details
     * `Device` is the central object most other LavaVK types are created
     * from. Given a chosen #GPUHardware and a list of requested
     * #QueueType values, it creates the logical `VkDevice`, retrieves a
     * `Queue` for each requested queue type, and lazily creates one
     * `CommandPool` per queue type as needed via #getCommandPool(). It also
     * exposes #submit() convenience overloads that build a `SubmitInfo`
     * from LavaVK synchronization objects and submit it on the appropriate
     * queue, so callers do not need to construct `VkSubmitInfo` by hand.
     * `Device` is move-only RAII: the underlying `VkDevice` is created in
     * the constructor and destroyed in the destructor.
     *
     * Example:
     * @code
     * LavaVK::Device device(
     *     selectedGPU,
     *     { LavaVK::QueueType::GRAPHICS, LavaVK::QueueType::PRESENT },
     *     &surface
     * );
     *
     * device.getCommandPool(LavaVK::QueueType::GRAPHICS).allocate(LavaVK::MAX_FRAMES_IN_FLIGHT);
     * // ... record a command buffer ...
     * device.submit(LavaVK::QueueType::GRAPHICS, frameIndex,
     *               { imageAvailable }, { LavaVK::PipelineStage::ColorAttachmentOutput },
     *               { renderFinished }, &inFlightFence);
     * @endcode
     */
    class Device {
    public:
        /**
         * @brief Constructs a LavaVK Device, selecting a physical GPU and creating a logical device.
         * @details Creates the `VkDevice` from @p gpu_hardware with one queue
         * per entry in @p requestedQueues, resolving each `QueueType` to a
         * concrete queue family index (using @p surface when a `PRESENT`
         * queue is requested) and retrieving the resulting `Queue` handles.
         * @param gpu_hardware The physical GPU to create the logical device from.
         * @param requestedQueues The set of queue types (graphics, compute, transfer, present, ...) to create.
         * @param surface Surface used to resolve presentation support; may be
         * `nullptr` if `QueueType::PRESENT` is not requested.
         * @throw std::runtime_error If device creation fails or a requested queue type cannot be satisfied.
         */
        explicit Device(const GPUHardware &gpu_hardware, const std::vector<QueueType> &requestedQueues,
                        const Surface *surface);

        /**
         * @brief Destructor. Destroys the managed `VkDevice` if valid.
         * @details Destroys owned command pools before destroying the
         * logical device itself; a moved-from `Device` is a no-op.
         */
        ~Device();

        /// @name Deleted Copy Operations
        /// @{
        Device(const Device &) = delete;

        Device &operator=(const Device &) = delete;

        /// @}

        /**
         * @brief Move constructor. Transfers logical device and queue ownership from another `Device`.
         * @param other The device being moved from; left in an empty, destructible state.
         */
        Device(Device &&other) noexcept;

        /**
         * @brief Move assignment operator. Destroys current resources and acquires ownership from `other`.
         * @param other The device being moved from; left in an empty, destructible state.
         * @return Reference to this device.
         */
        Device &operator=(Device &&other) noexcept;

        /**
         * @brief Gets the native Vulkan logical device handle (`VkDevice`).
         * @return The underlying `VkDevice` handle, or `VK_NULL_HANDLE` if unconstructed/moved-from.
         */
        [[nodiscard]] VkDevice native() const { return m_device; }

        /**
         * @brief Gets the native Vulkan physical device handle (`VkPhysicalDevice`).
         * @return The underlying `VkPhysicalDevice` this logical device was created from.
         */
        [[nodiscard]] VkPhysicalDevice physical() const { return m_physicalDevice; }

        /**
         * @brief Gets a specific Queue by QueueType.
         * @param type The queue type to retrieve (must have been requested at construction).
         * @return Const reference to the matching #Queue.
         * @throw std::out_of_range If @p type was not requested when the device was created.
         */
        [[nodiscard]] const Queue &getQueue(QueueType type) const;

        /**
         * @brief Gets a specific Queue by QueueType.
         * @param type The queue type to retrieve (must have been requested at construction).
         * @return Mutable reference to the matching #Queue.
         * @throw std::out_of_range If @p type was not requested when the device was created.
         */
        [[nodiscard]] Queue &getQueue(QueueType type);

        /**
         * @brief Gets queue family index for a specific QueueType.
         * @param type The queue type to look up.
         * @return Index of the queue family backing @p type.
         * @throw std::out_of_range If @p type was not requested when the device was created.
         */
        [[nodiscard]] uint32_t getQueueFamily(QueueType type) const;

        /**
         * @brief Waits till command pool is finished.
         * @details Blocks the calling thread until all queues on this
         * device have completed all previously submitted work. Wraps
         * `vkDeviceWaitIdle`.
         */
        void waitIdle() const { vkDeviceWaitIdle(native()); }

        /**
         * @brief Return CommandPool of qiven queue type.
         * @details Lazily creates a `CommandPool` bound to @p queueType's
         * queue family the first time it is requested, and returns the
         * same pool on subsequent calls.
         *
         * @param queueType Type of command pool to be returned.
         * @return Reference to the (possibly newly created) #CommandPool for @p queueType.
         */
        [[nodiscard]] CommandPool &getCommandPool(QueueType queueType);

        /**
         * @brief Submits a CommandBuffer.
         * @details Builds a `SubmitInfo` from the given semaphores, stages,
         * and command buffer, then submits it on the queue associated with
         * @p queueType.
         *
         * @param queueType Type of the CommandPool.
         * @param cmdBuffer CommandBuffer reference.
         * @param waitSemaphores Semaphores to wait.
         * @param waitStages Pipeline stages at which each corresponding wait semaphore applies.
         * @param signalSemaphores Semaphores to be signaled.
         * @param fence Optional fence to signal once the submitted work completes; may be `nullptr`.
         * @throw std::runtime_error If submission fails.
         */
        void submit(
            QueueType queueType,
            const CommandBuffer &cmdBuffer,
            const std::vector<std::reference_wrapper<const Semaphore> > &waitSemaphores = {},
            const std::vector<PipelineStage> &waitStages = {},
            const std::vector<std::reference_wrapper<const Semaphore> > &signalSemaphores = {},
            const Fence *fence = nullptr
        );

        /**
         * @brief Submits a CommandBuffer by index in CommandPool.
         * @details Equivalent to the #submit(QueueType, const CommandBuffer&, ...)
         * overload, but looks the command buffer up by index within
         * @p queueType's command pool via `CommandPool::retrieve`.
         *
         * @param queueType Type of the CommandPool.
         * @param commandBufferIndex CommandBuffer index.
         * @param waitSemaphores Semaphores to wait.
         * @param waitStages Pipeline stages at which each corresponding wait semaphore applies.
         * @param signalSemaphores Semaphores to be signaled.
         * @param fence Optional fence to signal once the submitted work completes; may be `nullptr`.
         * @throw std::out_of_range If @p commandBufferIndex is out of range for the pool.
         * @throw std::runtime_error If submission fails.
         */
        void submit(
            QueueType queueType,
            size_t commandBufferIndex,
            const std::vector<std::reference_wrapper<const Semaphore> > &waitSemaphores = {},
            const std::vector<PipelineStage> &waitStages = {},
            const std::vector<std::reference_wrapper<const Semaphore> > &signalSemaphores = {},
            const Fence *fence = nullptr
        );

    private:
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;

        std::unordered_map<QueueType, Queue> m_queues;
        std::unordered_map<QueueType, uint32_t> m_queueFamilies;
        std::unordered_map<QueueType, std::unique_ptr<CommandPool> > m_commandPools;
    };
} // namespace LavaVK

#endif // LAVAVK_DEVICE_H