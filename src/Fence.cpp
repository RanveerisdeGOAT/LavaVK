#include "../include/LavaVK/Fence.hpp"
#include "../include/LavaVK/Device.hpp"
#include <utility>

namespace LavaVK
{
    Fence::Fence(Device& device, bool signaled) : m_device(device.native())
    {
        VkFenceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        createInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

        if (vkCreateFence(m_device, &createInfo, nullptr, &m_fence) != VK_SUCCESS)
        {
            throw std::runtime_error("[LavaVK ERROR] Failed to create VkFence.");
        }
    }

    Fence::~Fence()
    {
        if (m_fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(m_device, m_fence, nullptr);
        }
    }

    Fence::Fence(Fence&& other) noexcept 
        : m_device(other.m_device), m_fence(other.m_fence)
    {
        other.m_device = VK_NULL_HANDLE;
        other.m_fence = VK_NULL_HANDLE;
    }

    Fence& Fence::operator=(Fence&& other) noexcept
    {
        if (this != &other)
        {
            if (m_fence != VK_NULL_HANDLE)
            {
                vkDestroyFence(m_device, m_fence, nullptr);
            }
            m_device = other.m_device;
            m_fence = other.m_fence;
            other.m_device = VK_NULL_HANDLE;
            other.m_fence = VK_NULL_HANDLE;
        }
        return *this;
    }

    VkResult Fence::wait(uint64_t timeout) const
    {
        return vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, timeout);
    }

    VkResult Fence::reset() const
    {
        return vkResetFences(m_device, 1, &m_fence);
    }
}