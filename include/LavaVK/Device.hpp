#ifndef LAVAVK_DEVICE_H
#define LAVAVK_DEVICE_H

#include <vulkan/vulkan.h>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "Queue.hpp"
#include "Surface.hpp"

namespace LavaVK
{
    class Surface;
    enum class QueueType : uint32_t;
    class Instance;

    /**
     * @brief Configuration options for selecting and initializing a physical/logical Vulkan GPU device.
     */
    enum class GPUType
    {
        Other,
        Integrated,
        Discrete,
        Virtual,
        CPU
    };

    class GPUHardware
    {
    public:
        GPUHardware() = default;

        explicit GPUHardware(VkPhysicalDevice device);

        /**
         * @brief Returns every Vulkan compatible GPU in the system.
         */
        static std::vector<GPUHardware> enumerate(const Instance& instance);

        /**
         * @brief Returns the GPU name.
         */
        [[nodiscard]] const std::string& name() const;

        /**
         * @brief Returns the GPU type.
         */
        [[nodiscard]] GPUType type() const;

        /**
         * @brief Returns the Vulkan API version.
         */
        [[nodiscard]] uint32_t apiVersion() const;

        /**
         * @brief Returns the Vulkan driver version.
         */
        [[nodiscard]] uint32_t driverVersion() const;

        /**
         * @brief Returns the Vulkan vendor ID.
         */
        [[nodiscard]] uint32_t vendorID() const;

        /**
         * @brief Returns the Vulkan device ID.
         */
        [[nodiscard]] uint32_t deviceID() const;

        /**
         * @brief Finds a queue family supporting the requested operations.
         */
        [[nodiscard]] uint32_t findQueueFamily(QueueType type, const Surface* surface) const;

        /**
         * @brief Returns the underlying Vulkan handle.
         */
        [[nodiscard]] VkPhysicalDevice native() const;

        /**
         * @brief Finds a queue family supporting surface presentation.
         * @param surface The surface to present to.
         * @return Queue family index supporting presentation.
         * @throw std::runtime_error if no queue family supports presentation.
         */
        [[nodiscard]] uint32_t findPresentQueueFamily(const Surface& surface) const;

        /**
         * @brief Checks if a specific queue family supports presenting to a given surface.
         */
        [[nodiscard]] bool supportsPresentation(uint32_t queueFamilyIndex, const Surface& surface) const;

        /**
         * @brief Checks if the physical GPU supports swapchain rendering on a given surface.
         */
        [[nodiscard]] bool isSurfaceSupported(const Surface& surface) const;

        [[nodiscard]]  static GPUHardware selectOptimalGPU(const Instance& instance, const Surface& surface);

        [[nodiscard]] SurfaceCapabilities getSurfaceCapabilities(const Surface& surface) const;
    private:
        friend class Device;

        VkPhysicalDevice m_device = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties m_properties{};
    };

    /**
     * @brief Manages the Vulkan physical device selection, logical device instantiation (`VkDevice`),
     * and graphics queue retrieval.
     */
    class Device
    {
    public:
        /**
         * @brief Constructs a LavaVK Device, selecting a physical GPU and creating a logical device.
         */
        explicit Device(const GPUHardware& gpu_hardware, const std::vector<QueueType>& requestedQueues, const Surface* surface);

        /**
         * @brief Destructor. Destroys the managed `VkDevice` if valid.
         */
        ~Device();

        /// @name Deleted Copy Operations
        /// @{
        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;
        /// @}

        /**
         * @brief Move constructor. Transfers logical device and queue ownership from another `Device`.
         */
        Device(Device&& other) noexcept;

        /**
         * @brief Move assignment operator. Destroys current resources and acquires ownership from `other`.
         */
        Device& operator=(Device&& other) noexcept;

        /**
         * @brief Gets the native Vulkan logical device handle (`VkDevice`).
         */
        [[nodiscard]] VkDevice native() const { return m_device; }

        /**
         * @brief Gets the native Vulkan physical device handle (`VkPhysicalDevice`).
         */
        [[nodiscard]] VkPhysicalDevice physical() const { return m_physicalDevice; }

        /**
         * @brief Gets a specific Queue by QueueType.
         */
        [[nodiscard]] const Queue& getQueue(QueueType type) const;
        [[nodiscard]] Queue& getQueue(QueueType type);

        /**
         * @brief Gets queue family index for a specific QueueType.
         */
        [[nodiscard]] uint32_t getQueueFamily(QueueType type) const;

        /**
         * @brief Convenience helper for graphics queue.
         */
        [[nodiscard]] const Queue& graphicsQueue() const { return getQueue(QueueType::GRAPHICS); }

    private:

        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;

        std::unordered_map<QueueType, Queue> m_queues;
        std::unordered_map<QueueType, uint32_t> m_queueFamilies;
    };

} // namespace LavaVK

#endif // LAVAVK_DEVICE_H