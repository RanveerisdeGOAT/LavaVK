#include "LavaVK/Buffer.hpp"

#include "LavaVK/Command.hpp"
#include "LavaVK/Pipeline.hpp"
#include "LavaVK/Queue.hpp"
#include "LavaVK/Texture.hpp"

namespace LavaVK {

    namespace {
        VkMemoryPropertyFlags getMemoryFlags(LavaVK::MemoryUsage usage) {
            switch (usage) {
                case LavaVK::MemoryUsage::GPU:
                    return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

                case LavaVK::MemoryUsage::CPU:
                case LavaVK::MemoryUsage::CPU_TO_GPU:
                    return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

                case LavaVK::MemoryUsage::GPU_TO_CPU:
                    return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
            }
            return 0;
        }
    }

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

    uint32_t Buffer::findMemoryType(VkPhysicalDevice physical, uint32_t typeFilter, VkFlags requiredProperties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physical, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            // Corrected parentheses around bitwise AND:
            bool supportsType = (typeFilter & (1 << i)) != 0;
            bool hasAllProperties = (memProperties.memoryTypes[i].propertyFlags & requiredProperties) == requiredProperties;

            if (supportsType && hasAllProperties) {
                return i;
            }
        }

        throw std::runtime_error("LavaVK::Buffer - Failed to find suitable memory type!");
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
        VkMemoryPropertyFlags memoryProperties = getMemoryFlags(m_memoryUsage);

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
            // Properly release existing Vulkan resources
            if (m_device) {
                if (m_mapped) { unmap(); }
                if (m_buffer != VK_NULL_HANDLE) { vkDestroyBuffer(m_device->native(), m_buffer, nullptr); }
                if (m_memory != VK_NULL_HANDLE) { vkFreeMemory(m_device->native(), m_memory, nullptr); }
            }

            // Steal state from other
            m_device = other.m_device;
            m_buffer = other.m_buffer;
            m_memory = other.m_memory;
            m_mapped = other.m_mapped;
            m_size = other.m_size;
            m_usage = other.m_usage;
            m_memoryUsage = other.m_memoryUsage;

            // Reset other
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

    void Buffer::copyToBuffer(Buffer &dstBuffer) const {
        // 1. Allocate a temporary command buffer from the graphics/transfer pool
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_device->getCommandPool(LavaVK::QueueType::GRAPHICS).native();
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(m_device->native(), &allocInfo, &cmd);

        // 2. Begin recording single-time commands
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        // 3. Record the buffer copy region
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = m_size;

        vkCmdCopyBuffer(cmd, native(), dstBuffer.native(), 1, &copyRegion);

        vkEndCommandBuffer(cmd);

        // 4. Submit to GPU queue and wait until complete
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        VkQueue graphicsQueue = m_device->getQueue(LavaVK::QueueType::GRAPHICS).native();
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue); // Wait for transfer to complete

        // 5. Free temporary command buffer
        vkFreeCommandBuffers(m_device->native(), m_device->getCommandPool(LavaVK::QueueType::GRAPHICS).native(), 1, &cmd);
    }
} // namespace LavaVK
