#include "../include/LavaVK/Device.hpp"
#include "../include/LavaVK/Instance.hpp"
#include "../include/LavaVK/Queue.hpp"

#include <stdexcept>
#include <vector>
#include <utility>
#include <set>

#include "LavaVK/LavaVK.hpp"
#include "LavaVK/Surface.hpp"

namespace LavaVK {
    namespace {
        GPUType convertType(VkPhysicalDeviceType type) {
            switch (type) {
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    return GPUType::Discrete;

                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    return GPUType::Integrated;

                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    return GPUType::Virtual;

                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    return GPUType::CPU;

                default:
                    return GPUType::Other;
            }
        }
    }

    GPUHardware GPUHardware::selectOptimalGPU(const Instance &instance, const Surface &surface) {
        auto gpus = GPUHardware::enumerate(instance);

        for (const auto &gpu: gpus) {
            // Ensure the GPU has a presentation queue and adequate surface formats/present modes
            if (gpu.isSurfaceSupported(surface)) {
                // Prefer discrete GPU
                if (gpu.type() == GPUType::Discrete) {
                    return gpu;
                }
            }
        }

        // Fallback to first compatible GPU
        for (const auto &gpu: gpus) {
            if (gpu.isSurfaceSupported(surface)) {
                return gpu;
            }
        }

        LAVAVK_ERROR("[LavaVK ERROR] No suitable GPU supports the target surface.");
    }

    GPUHardware::GPUHardware(VkPhysicalDevice device)
        : m_device(device) {
        vkGetPhysicalDeviceProperties(m_device, &m_properties);
    }

    std::vector<GPUHardware> GPUHardware::enumerate(const Instance &instance) {
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(instance.native(), &count, nullptr);

        if (count == 0)
            LAVAVK_ERROR("[LavaVK ERROR] No Vulkan compatible GPUs found.");

        std::vector<VkPhysicalDevice> physicalDevices(count);
        vkEnumeratePhysicalDevices(instance.native(), &count, physicalDevices.data());

        std::vector<GPUHardware> result;
        result.reserve(count);

        for (auto device: physicalDevices)
            result.emplace_back(device);

        return result;
    }

    const std::string &GPUHardware::name() const {
        static std::string name;
        name = m_properties.deviceName;
        return name;
    }

    GPUType GPUHardware::type() const {
        return convertType(m_properties.deviceType);
    }

    uint32_t GPUHardware::apiVersion() const {
        return m_properties.apiVersion;
    }

    uint32_t GPUHardware::driverVersion() const {
        return m_properties.driverVersion;
    }

    uint32_t GPUHardware::vendorID() const {
        return m_properties.vendorID;
    }

    uint32_t GPUHardware::deviceID() const {
        return m_properties.deviceID;
    }

    uint32_t GPUHardware::findQueueFamily(QueueType type, const Surface *surface) const {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_device, &count, nullptr);

        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(m_device, &count, families.data());

        for (uint32_t i = 0; i < count; ++i) {
            // Special case: Presentation Queue check
            if (type == QueueType::PRESENT) {
                if (!surface) {
                    LAVAVK_ERROR("[LavaVK ERROR] Cannot query QueueType::PRESENT without a valid Surface.");
                }

                VkBool32 presentSupport = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(m_device, i, surface->native(), &presentSupport);

                if (presentSupport == VK_TRUE) {
                    return i;
                }
            } else {
                // Standard Vulkan hardware queue flag check
                VkQueueFlags requiredFlags = static_cast<VkQueueFlags>(type);
                if ((families[i].queueFlags & requiredFlags) == requiredFlags) {
                    return i;
                }
            }
        }

        LAVAVK_ERROR("[LavaVK ERROR] Failed to find requested QueueType family on GPU.");
    }

    VkPhysicalDevice GPUHardware::native() const {
        return m_device;
    }

    bool GPUHardware::supportsPresentation(uint32_t queueFamilyIndex, const Surface &surface) const {
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_device, queueFamilyIndex, surface.native(), &presentSupport);
        return presentSupport == VK_TRUE;
    }

    uint32_t GPUHardware::findPresentQueueFamily(const Surface &surface) const {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_device, &count, nullptr);

        for (uint32_t i = 0; i < count; ++i) {
            if (supportsPresentation(i, surface)) {
                return i;
            }
        }

        LAVAVK_ERROR("[LavaVK ERROR] GPU hardware does not support presentation on the given surface.");
    }

    bool GPUHardware::isSurfaceSupported(const Surface &surface) const {
        uint32_t formatCount = 0;
        uint32_t presentModeCount = 0;

        vkGetPhysicalDeviceSurfaceFormatsKHR(m_device, surface.native(), &formatCount, nullptr);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_device, surface.native(), &presentModeCount, nullptr);

        return formatCount > 0 && presentModeCount > 0;
    }

    SurfaceCapabilities GPUHardware::getSurfaceCapabilities(const Surface &surface) const {
        SurfaceCapabilities result{};
        VkPhysicalDevice rawGpu = m_device;
        VkSurfaceKHR rawSurface = surface.native();

        // 1. Query Capabilities
        VkSurfaceCapabilitiesKHR vkCaps{};
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(rawGpu, rawSurface, &vkCaps) != VK_SUCCESS) {
            LAVAVK_ERROR("[LavaVK ERROR] Failed to query surface capabilities.");
        }

        result.minImageCount = vkCaps.minImageCount;
        result.maxImageCount = vkCaps.maxImageCount;

        result.currentExtent = {vkCaps.currentExtent.width, vkCaps.currentExtent.height};
        result.minImageExtent = {vkCaps.minImageExtent.width, vkCaps.minImageExtent.height};
        result.maxImageExtent = {vkCaps.maxImageExtent.width, vkCaps.maxImageExtent.height};

        result.currentTransform = static_cast<uint32_t>(vkCaps.currentTransform);
        result.supportedTransforms = static_cast<uint32_t>(vkCaps.supportedTransforms);

        // 2. Query Surface Formats
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(rawGpu, rawSurface, &formatCount, nullptr);

        if (formatCount > 0) {
            std::vector<VkSurfaceFormatKHR> vkFormats(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(rawGpu, rawSurface, &formatCount, vkFormats.data());

            result.formats.reserve(formatCount);
            for (const auto &fmt: vkFormats) {
                result.formats.push_back({
                    static_cast<SurfaceFormat>(fmt.format),
                    static_cast<ColorSpace>(fmt.colorSpace)
                });
            }
        }

        // 3. Query Present Modes
        uint32_t modeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(rawGpu, rawSurface, &modeCount, nullptr);

        if (modeCount > 0) {
            std::vector<VkPresentModeKHR> vkModes(modeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(rawGpu, rawSurface, &modeCount, vkModes.data());

            result.presentModes.reserve(modeCount);
            for (const auto &mode: vkModes) {
                result.presentModes.push_back(static_cast<PresentMode>(mode));
            }
        }

        return result;
    }

    Device::Device(const GPUHardware &gpu_hardware,
                   const std::vector<QueueType> &requestedQueues,
                   const Surface *surface) {
        m_physicalDevice = gpu_hardware.native();

        std::set<uint32_t> uniqueFamilies;

        for (auto type: requestedQueues) {
            // Pass surface to resolve QueueType::PRESENT if needed
            uint32_t family = gpu_hardware.findQueueFamily(type, surface);
            m_queueFamilies[type] = family;
            uniqueFamilies.insert(family);
        }


        float priority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

        for (uint32_t familyIndex: uniqueFamilies) {
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = familyIndex;
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = &priority;
            queueCreateInfos.push_back(queueInfo);
        }

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        std::vector<const char *> extensions;
        if (surface != nullptr) {
            extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }

        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
            LAVAVK_ERROR("[LavaVK ERROR] Failed to create logical device.");
        }

        for (auto type: requestedQueues) {
            m_queues[type] = Queue(*this, m_queueFamilies[type]);
        }

        for (QueueType queueType: requestedQueues) {
            // Avoid creating duplicate pools if the same QueueType is passed twice
            if (m_commandPools.find(queueType) == m_commandPools.end()) {
                m_commandPools[queueType] = std::make_unique<CommandPool>(
                    *this,
                    queueType,
                    false,
                    true
                );
            }
        }
    }

    Device::~Device() {
        if (m_device != VK_NULL_HANDLE) {
            m_commandPools.clear();
            vkDestroyDevice(m_device, nullptr);
        }
    }

    Device::Device(Device &&other) noexcept
        : m_physicalDevice(other.m_physicalDevice),
          m_device(other.m_device),
          m_queues(std::move(other.m_queues)),
          m_queueFamilies(std::move(other.m_queueFamilies)) {
        other.m_physicalDevice = VK_NULL_HANDLE;
        other.m_device = VK_NULL_HANDLE;
    }

    Device &Device::operator=(Device &&other) noexcept {
        if (this != &other) {
            if (m_device != VK_NULL_HANDLE)
                vkDestroyDevice(m_device, nullptr);

            m_physicalDevice = other.m_physicalDevice;
            m_device = other.m_device;
            m_queues = std::move(other.m_queues);
            m_queueFamilies = std::move(other.m_queueFamilies);

            other.m_physicalDevice = VK_NULL_HANDLE;
            other.m_device = VK_NULL_HANDLE;
        }
        return *this;
    }

    const Queue &Device::getQueue(QueueType type) const {
        auto it = m_queues.find(type);
        if (it == m_queues.end())
            LAVAVK_ERROR("Requested queue type not initialized on this device.");
        return it->second;
    }

    Queue &Device::getQueue(QueueType type) {
        auto it = m_queues.find(type);
        if (it == m_queues.end())
            LAVAVK_ERROR("Requested queue type not initialized on this device.");
        return it->second;
    }

    uint32_t Device::getQueueFamily(QueueType type) const {
        auto it = m_queueFamilies.find(type);
        if (it == m_queueFamilies.end())
            LAVAVK_ERROR("Requested queue family not initialized on this device.");
        return it->second;
    }

    CommandPool &Device::getCommandPool(QueueType queueType) {
        // Lazy-allocate pool if it doesn't exist yet for this queue
        if (m_commandPools.find(queueType) == m_commandPools.end()) {
            m_commandPools[queueType] = std::make_unique<CommandPool>(
                *this,
                queueType,
                false, // transient
                true // resetIndividualBuffers
            );
        }
        return *m_commandPools[queueType];
    }

    void Device::submit(
    QueueType queueType,
    const CommandBuffer &cmdBuffer,
    const std::vector<std::reference_wrapper<const Semaphore>> &waitSemaphores,
    const std::vector<PipelineStage> &waitStages, // Changed to vector
    const std::vector<std::reference_wrapper<const Semaphore>> &signalSemaphores,
    const Fence *fence) {

    VkCommandBuffer rawCmd = cmdBuffer.native();

    VkSubmitInfo submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &rawCmd;

    // Convert wait semaphores
    std::vector<VkSemaphore> rawWaitSemaphores;
    rawWaitSemaphores.reserve(waitSemaphores.size());
    for (const auto &semRef : waitSemaphores) {
        rawWaitSemaphores.push_back(semRef.get().native());
    }

    // Convert wait stages enum to raw VkPipelineStageFlags
    std::vector<VkPipelineStageFlags> rawWaitStages;
    rawWaitStages.reserve(waitStages.size());
    for (const auto &stage : waitStages) {
        rawWaitStages.push_back(static_cast<VkPipelineStageFlags>(stage));
    }

    // Convert signal semaphores
    std::vector<VkSemaphore> rawSignalSemaphores;
    rawSignalSemaphores.reserve(signalSemaphores.size());
    for (const auto &semRef : signalSemaphores) {
        rawSignalSemaphores.push_back(semRef.get().native());
    }

    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(rawWaitSemaphores.size());
    submitInfo.pWaitSemaphores    = rawWaitSemaphores.data();
    submitInfo.pWaitDstStageMask  = rawWaitStages.data(); // Pass pointer to contiguous flags array

    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(rawSignalSemaphores.size());
    submitInfo.pSignalSemaphores   = rawSignalSemaphores.data();

    VkQueue queue = getQueue(queueType).native();
    VkFence rawFence = fence ? fence->native() : VK_NULL_HANDLE;

    if (vkQueueSubmit(queue, 1, &submitInfo, rawFence) != VK_SUCCESS) {
        throw std::runtime_error("Device::submit - Failed to submit command buffer!");
    }
}

    void Device::submit(
        QueueType queueType,
        size_t commandBufferIndex,
        const std::vector<std::reference_wrapper<const Semaphore> > &waitSemaphores,
        const std::vector<PipelineStage> &waitStages,
        const std::vector<std::reference_wrapper<const Semaphore> > &signalSemaphores,
        const Fence *fence) {
        const CommandBuffer &cmdBuffer = getCommandPool(queueType).retrieve(commandBufferIndex);
        submit(queueType, cmdBuffer, waitSemaphores, waitStages, signalSemaphores, fence);
    }
} // namespace LavaVK
