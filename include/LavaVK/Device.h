#ifndef LAVAVK_DEVICE_H
#define LAVAVK_DEVICE_H

#include <vulkan/vulkan.h>
#include <cstdint>

namespace LavaVK
{
    class Instance;

    /**
     * @brief Configuration options for selecting and initializing a physical/logical Vulkan GPU device.
     */
    struct GPUDeviceCreateInfo
    {
        /** * @brief Index of a specific GPU in the system to use.
         * Set to `UINT32_MAX` (default) to let LavaVK automatically select an optimal GPU.
         */
        uint32_t deviceIndex = UINT32_MAX;

        /** * @brief Prefers a discrete GPU (`VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU`) over integrated graphics
         * when `deviceIndex` is not specified.
         */
        bool preferDiscreteGPU = true;
    };

    /**
     * @brief Manages the Vulkan physical device selection, logical device instantiation (`VkDevice`),
     * and graphics queue retrieval.
     *
     * Implements RAII semantics for automated lifecycle management of the logical device. Move operations
     * are supported, but copy operations are disabled to prevent duplicate destruction.
     */
    class Device
    {
    public:
        /**
         * @brief Constructs a LavaVK Device, selecting a physical GPU and creating a logical device.
         * * @param info Configuration structure specifying GPU selection strategy.
         * @param instance Valid reference to the parent LavaVK Instance.
         * * @throw std::runtime_error Thrown if no physical device is found, if queue selection fails,
         * or if logical device creation fails.
         */
        explicit Device(const GPUDeviceCreateInfo& info, const Instance& instance);

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
         * @param other The device instance to move from.
         */
        Device(Device&& other) noexcept;

        /**
         * @brief Move assignment operator. Destroys current resources and acquires ownership from `other`.
         * @param other The device instance to move from.
         * @return Reference to this `Device` instance.
         */
        Device& operator=(Device&& other) noexcept;

        /**
         * @brief Gets the native Vulkan logical device handle (`VkDevice`).
         * @return The underlying `VkDevice` handle.
         */
        [[nodiscard]]
        VkDevice native() const
        {
            return m_device;
        }

        /**
         * @brief Gets the native Vulkan physical device handle (`VkPhysicalDevice`).
         * @return The underlying `VkPhysicalDevice` handle.
         */
        [[nodiscard]]
        VkPhysicalDevice physical() const
        {
            return m_physicalDevice;
        }

        /**
         * @brief Gets the primary graphics queue retrieved during device creation.
         * @return The underlying `VkQueue` handle.
         */
        [[nodiscard]]
        VkQueue graphicsQueue() const
        {
            return m_graphicsQueue;
        }

        /**
         * @brief Gets the queue family index supporting graphics operations.
         * @return The queue family index.
         */
        [[nodiscard]]
        uint32_t graphicsFamily() const
        {
            return m_graphicsFamily;
        }

    private:
        /** @brief Handle to the physical GPU hardware. */
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

        /** @brief Handle to the logical Vulkan device connection. */
        VkDevice m_device = VK_NULL_HANDLE;

        /** @brief Handle to the graphics queue for submitting work. */
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;

        /** @brief Index of the queue family supporting graphics pipeline operations. */
        uint32_t m_graphicsFamily = 0;
    };

} // namespace LavaVK

#endif // LAVAVK_DEVICE_H