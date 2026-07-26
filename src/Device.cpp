#include "../include/LavaVK/Device.h"
#include "../include/LavaVK/Instance.h"
#include "../include/LavaVK/Queue.h"

#include <stdexcept>
#include <vector>
#include <utility>
#include <set>

namespace LavaVK
{

namespace
{
    GPUType convertType(VkPhysicalDeviceType type)
    {
        switch (type)
        {
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

GPUHardware::GPUHardware(VkPhysicalDevice device)
    : m_device(device)
{
    vkGetPhysicalDeviceProperties(m_device, &m_properties);
}

std::vector<GPUHardware> GPUHardware::enumerate(const Instance& instance)
{
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(instance.native(), &count, nullptr);

    if (count == 0)
        throw std::runtime_error("[LavaVK ERROR] No Vulkan compatible GPUs found.");

    std::vector<VkPhysicalDevice> physicalDevices(count);
    vkEnumeratePhysicalDevices(instance.native(), &count, physicalDevices.data());

    std::vector<GPUHardware> result;
    result.reserve(count);

    for (auto device : physicalDevices)
        result.emplace_back(device);

    return result;
}

const std::string& GPUHardware::name() const
{
    static std::string name;
    name = m_properties.deviceName;
    return name;
}

GPUType GPUHardware::type() const
{
    return convertType(m_properties.deviceType);
}

uint32_t GPUHardware::apiVersion() const
{
    return m_properties.apiVersion;
}

uint32_t GPUHardware::driverVersion() const
{
    return m_properties.driverVersion;
}

uint32_t GPUHardware::vendorID() const
{
    return m_properties.vendorID;
}

uint32_t GPUHardware::deviceID() const
{
    return m_properties.deviceID;
}

uint32_t GPUHardware::findQueueFamily(QueueType type) const
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_device, &count, nullptr);

    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_device, &count, families.data());

    const VkQueueFlags required = static_cast<VkQueueFlags>(type);

    for (uint32_t i = 0; i < count; ++i)
    {
        if ((families[i].queueFlags & required) == required)
            return i;
    }

    throw std::runtime_error("[LavaVK ERROR] Graphics queue not found.");
}

VkPhysicalDevice GPUHardware::native() const
{
    return m_device;
}

Device::Device(const GPUHardware& gpu_hardware, const std::vector<QueueType>& requestedQueues)
{
    m_physicalDevice = gpu_hardware.native();

    // 1. Resolve Queue Families and Deduplicate Unique Family Indices
    std::set<uint32_t> uniqueFamilies;

    for (auto type : requestedQueues)
    {
        uint32_t family = gpu_hardware.findQueueFamily(type);
        m_queueFamilies[type] = family;
        uniqueFamilies.insert(family);
    }

    // 2. Prepare Queue Create Infos
    float priority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    for (uint32_t familyIndex : uniqueFamilies)
    {
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = familyIndex;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;
        queueCreateInfos.push_back(queueInfo);
    }

    // 3. Create Logical Device
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = extensions;

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create logical device.");
    }

    // 4. Instantiate Queue Objects
    for (auto type : requestedQueues)
    {
        m_queues[type] = Queue(*this, m_queueFamilies[type]);
    }
}

Device::~Device()
{
    if (m_device != VK_NULL_HANDLE)
        vkDestroyDevice(m_device, nullptr);
}

Device::Device(Device&& other) noexcept
    : m_physicalDevice(other.m_physicalDevice),
      m_device(other.m_device),
      m_queues(std::move(other.m_queues)),
      m_queueFamilies(std::move(other.m_queueFamilies))
{
    other.m_physicalDevice = VK_NULL_HANDLE;
    other.m_device = VK_NULL_HANDLE;
}

Device& Device::operator=(Device&& other) noexcept
{
    if (this != &other)
    {
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

const Queue& Device::getQueue(QueueType type) const
{
    auto it = m_queues.find(type);
    if (it == m_queues.end())
        throw std::runtime_error("Requested queue type not initialized on this device.");
    return it->second;
}

Queue& Device::getQueue(QueueType type)
{
    auto it = m_queues.find(type);
    if (it == m_queues.end())
        throw std::runtime_error("Requested queue type not initialized on this device.");
    return it->second;
}

uint32_t Device::getQueueFamily(QueueType type) const
{
    auto it = m_queueFamilies.find(type);
    if (it == m_queueFamilies.end())
        throw std::runtime_error("Requested queue family not initialized on this device.");
    return it->second;
}

} // namespace LavaVK