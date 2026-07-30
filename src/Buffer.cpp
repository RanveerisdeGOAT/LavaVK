#include "LavaVK/Buffer.hpp"

#include "LavaVK/Pipeline.hpp"
#include "LavaVK/Texture.hpp"

namespace LavaVK {

    Framebuffer::Framebuffer(
        Device &device,
        RenderPass &renderPass,
        const std::vector<Image *> &attachments)
        : m_attachments(attachments),
          m_device(device) {
        if (attachments.empty()) {
            LAVAVK_ERROR("[LavaVK ERROR] Cannot create Framebuffer without attachments.");
        }

        std::vector<VkImageView> attachmentViews;
        attachmentViews.reserve(attachments.size());

        for (const Image *attachment: attachments) {
            if (!attachment || attachment->view() == VK_NULL_HANDLE) {
                LAVAVK_ERROR("[LavaVK ERROR] Invalid image attachment provided to Framebuffer.");
            }
            attachmentViews.push_back(attachment->view());
        }

        uint32_t width = attachments[0]->extent().width;
        uint32_t height = attachments[0]->extent().height;

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass.native(); // Using native() accessor from Pipeline.hpp
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
        framebufferInfo.pAttachments = attachmentViews.data();
        framebufferInfo.width = width;
        framebufferInfo.height = height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device.native(), &framebufferInfo, nullptr, &m_framebuffer) != VK_SUCCESS) {
            LAVAVK_ERROR("[LavaVK ERROR] Failed to create framebuffer.");
        }
    }

    Framebuffer::~Framebuffer() {
        vkDestroyFramebuffer(m_device.native(), native(), nullptr);
    }

    uint32_t Buffer::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                                    VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        LAVAVK_ERROR("LavaVK::Buffer - Failed to find suitable memory type!");
    }

    Buffer::Buffer(Device &device, const BufferCreateInfo &info)
        : m_device(&device), m_size(info.size), m_usage(info.usage), m_memoryUsage(info.memory) {
        if (m_size == 0) {
            throw std::invalid_argument("LavaVK::Buffer - Cannot create a buffer with size 0!");
        }

        // 1. Create Vulkan Buffer Handle
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = m_size;
        bufferInfo.usage = static_cast<VkBufferUsageFlags>(m_usage);
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(m_device->native(), &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS) {
            LAVAVK_ERROR("LavaVK::Buffer - Failed to create VkBuffer!");
        }

        // 2. Query Memory Requirements
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_device->native(), m_buffer, &memRequirements);

        // 3. Map MemoryUsage to VkMemoryPropertyFlags
        VkMemoryPropertyFlags memoryProperties = 0;
        switch (m_memoryUsage) {
            case MemoryUsage::GPU:
                memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                break;
            case MemoryUsage::CPU:
            case MemoryUsage::CPU_TO_GPU:
                memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                break;
            case MemoryUsage::GPU_TO_CPU:
                memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
                break;
        }

        // 4. Allocate Device Memory
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(m_device->physical(), memRequirements.memoryTypeBits,
                                                   memoryProperties);

        if (vkAllocateMemory(m_device->native(), &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
            vkDestroyBuffer(m_device->native(), m_buffer, nullptr);
            LAVAVK_ERROR("LavaVK::Buffer - Failed to allocate buffer memory!");
        }

        // 5. Bind Buffer to Memory
        vkBindBufferMemory(m_device->native(), m_buffer, m_memory, 0);
    }

    Buffer::~Buffer() {
        if (m_device) {
            if (m_mapped) {
                unmap();
            }
            if (m_buffer != VK_NULL_HANDLE) {
                vkDestroyBuffer(m_device->native(), m_buffer, nullptr);
                m_buffer = VK_NULL_HANDLE;
            }
            if (m_memory != VK_NULL_HANDLE) {
                vkFreeMemory(m_device->native(), m_memory, nullptr);
                m_memory = VK_NULL_HANDLE;
            }
        }
    }

    // Move Constructor
    Buffer::Buffer(Buffer &&other) noexcept
        : m_device(other.m_device),
          m_buffer(other.m_buffer),
          m_memory(other.m_memory),
          m_mapped(other.m_mapped),
          m_size(other.m_size),
          m_usage(other.m_usage),
          m_memoryUsage(other.m_memoryUsage) {
        other.m_device = nullptr;
        other.m_buffer = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_mapped = nullptr;
        other.m_size = 0;
    }

    // Move Assignment
    Buffer &Buffer::operator=(Buffer &&other) noexcept {
        if (this != &other) {
            // Clean up current resources
            this->~Buffer();

            m_device = other.m_device;
            m_buffer = other.m_buffer;
            m_memory = other.m_memory;
            m_mapped = other.m_mapped;
            m_size = other.m_size;
            m_usage = other.m_usage;
            m_memoryUsage = other.m_memoryUsage;

            other.m_device = nullptr;
            other.m_buffer = VK_NULL_HANDLE;
            other.m_memory = VK_NULL_HANDLE;
            other.m_mapped = nullptr;
            other.m_size = 0;
        }
        return *this;
    }

    void *Buffer::map() {
        if (!m_mapped && m_memory != VK_NULL_HANDLE) {
            if (vkMapMemory(m_device->native(), m_memory, 0, m_size, 0, &m_mapped) != VK_SUCCESS) {
                LAVAVK_ERROR("LavaVK::Buffer - Failed to map memory!");
            }
        }
        return m_mapped;
    }

    void Buffer::unmap() {
        if (m_mapped && m_device && m_memory != VK_NULL_HANDLE) {
            vkUnmapMemory(m_device->native(), m_memory);
            m_mapped = nullptr;
        }
    }

    void Buffer::upload(const void *data, size_t bytes) {
        if (bytes > m_size) {
            throw std::out_of_range("LavaVK::Buffer::upload - Upload size exceeds buffer capacity!");
        }

        bool wasMapped = (m_mapped != nullptr);
        void *ptr = wasMapped ? m_mapped : map();

        std::memcpy(ptr, data, bytes);

        if (!wasMapped) {
            unmap();
        }
    }
} // namespace LavaVK
