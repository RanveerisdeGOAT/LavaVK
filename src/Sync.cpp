#include "LavaVK/Sync.hpp"

#include "LavaVK/Core.hpp"
#include "LavaVK/Device.hpp"
#include "LavaVK/Instance.hpp"
#include "LavaVK/Error.hpp"

namespace LavaVK {
    Fence::Fence(Device &device, bool signaled) : m_device(device.native()) {
        VkFenceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        createInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

        if (vkCreateFence(m_device, &createInfo, nullptr, &m_fence) != VK_SUCCESS) {
            LAVAVK_ERROR("[LavaVK ERROR] Failed to create VkFence.");
        }
    }

    Fence::~Fence() {
        if (m_fence != VK_NULL_HANDLE) {
            vkDestroyFence(m_device, m_fence, nullptr);
        }
    }

    Fence::Fence(Fence &&other) noexcept
        : m_device(other.m_device), m_fence(other.m_fence) {
        other.m_device = VK_NULL_HANDLE;
        other.m_fence = VK_NULL_HANDLE;
    }

    Fence &Fence::operator=(Fence &&other) noexcept {
        if (this != &other) {
            if (m_fence != VK_NULL_HANDLE) {
                vkDestroyFence(m_device, m_fence, nullptr);
            }
            m_device = other.m_device;
            m_fence = other.m_fence;
            other.m_device = VK_NULL_HANDLE;
            other.m_fence = VK_NULL_HANDLE;
        }
        return *this;
    }

    Result Fence::wait(uint64_t timeout) const {
        return vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, timeout);
    }

    Result Fence::reset() const {
        return vkResetFences(m_device, 1, &m_fence);
    }

    Semaphore::Semaphore(Device &device)
        : m_device(device.native()) {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if (vkCreateSemaphore(
                m_device,
                &info,
                nullptr,
                &m_semaphore) != VK_SUCCESS) {
            LAVAVK_ERROR("[LavaVK ERROR] Failed to create VkSemaphore.");
        }
    }

    Semaphore::~Semaphore() {
        if (m_semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(
                m_device,
                m_semaphore,
                nullptr);
        }
    }

    Semaphore::Semaphore(Semaphore &&other) noexcept
        : m_device(other.m_device),
          m_semaphore(other.m_semaphore) {
        other.m_device = VK_NULL_HANDLE;
        other.m_semaphore = VK_NULL_HANDLE;
    }

    Semaphore &Semaphore::operator=(Semaphore &&other) noexcept {
        if (this != &other) {
            if (m_semaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(
                    m_device,
                    m_semaphore,
                    nullptr);
            }

            m_device = other.m_device;
            m_semaphore = other.m_semaphore;

            other.m_device = VK_NULL_HANDLE;
            other.m_semaphore = VK_NULL_HANDLE;
        }

        return *this;
    }
}
