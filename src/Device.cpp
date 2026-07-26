#include "../include/LavaVK/Device.h"
#include "../include/LavaVK/Instance.h"

#include <stdexcept>
#include <vector>

namespace LavaVK
{

namespace
{

uint32_t findGraphicsQueueFamily(VkPhysicalDevice gpu)
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, nullptr);

    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, families.data());

    for (uint32_t i = 0; i < count; i++)
    {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            return i;
    }

    throw std::runtime_error("[LavaVK ERROR] Failed to find a graphics queue family.");
}

VkPhysicalDevice chooseDevice(const GPUDeviceCreateInfo& info, const Instance& instance)
{
    // Enumerate GPUs
    uint32_t gpuCount = 0;
    vkEnumeratePhysicalDevices(instance.native(), &gpuCount, nullptr);

    if (gpuCount == 0)
        throw std::runtime_error("[LavaVK ERROR] No Vulkan-compatible GPU found.");
    std::vector<VkPhysicalDevice> devices(gpuCount);

    vkEnumeratePhysicalDevices(instance.native(), &gpuCount, devices.data());
    if (info.deviceIndex != UINT32_MAX)
    {
        return devices[info.deviceIndex];
    }


    if (info.preferDiscreteGPU)
    {
        for (auto gpu : devices)
        {
            VkPhysicalDeviceProperties props{};
            vkGetPhysicalDeviceProperties(gpu, &props);
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                return gpu;
        }
    }


    return devices.front();
}

}

Device::Device(const GPUDeviceCreateInfo& GPUinfo, const Instance& instance)
{
    m_physicalDevice = chooseDevice(GPUinfo, instance);

    // Find graphics queue
    m_graphicsFamily = findGraphicsQueueFamily(m_physicalDevice);

    float priority = 1.0f;

    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = m_graphicsFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    #ifndef NDEBUG
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueInfo;

    // Device extensions (we'll need this for swapchains)
    const char* extensions[] =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = extensions;
    #endif

    if (vkCreateDevice(
        m_physicalDevice,
        &createInfo,
        nullptr,
        &m_device) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create logical device.");
    }

    vkGetDeviceQueue(
        m_device,
        m_graphicsFamily,
        0,
        &m_graphicsQueue);
}

Device::~Device()
{
    if (m_device != VK_NULL_HANDLE)
        vkDestroyDevice(m_device, nullptr);
}

Device::Device(Device&& other) noexcept
{
    m_physicalDevice = other.m_physicalDevice;
    m_device = other.m_device;
    m_graphicsQueue = other.m_graphicsQueue;
    m_graphicsFamily = other.m_graphicsFamily;

    other.m_physicalDevice = VK_NULL_HANDLE;
    other.m_device = VK_NULL_HANDLE;
    other.m_graphicsQueue = VK_NULL_HANDLE;
    other.m_graphicsFamily = 0;
}

Device& Device::operator=(Device&& other) noexcept
{
    if (this != &other)
    {
        if (m_device != VK_NULL_HANDLE)
            vkDestroyDevice(m_device, nullptr);

        m_physicalDevice = other.m_physicalDevice;
        m_device = other.m_device;
        m_graphicsQueue = other.m_graphicsQueue;
        m_graphicsFamily = other.m_graphicsFamily;

        other.m_physicalDevice = VK_NULL_HANDLE;
        other.m_device = VK_NULL_HANDLE;
        other.m_graphicsQueue = VK_NULL_HANDLE;
        other.m_graphicsFamily = 0;
    }

    return *this;
}

} // namespace LavaVK