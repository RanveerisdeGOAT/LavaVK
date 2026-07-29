#include "LavaVK/Buffer.hpp"

#include <cstring>

#include "LavaVK/Device.hpp"

#include <stdexcept>
#include <utility>

#include "LavaVK/LavaVK.hpp"

namespace LavaVK {
    namespace {
        VkImageType toVkImageType(ImageType type) {
            switch (type) {
                case ImageType::IMAGE_1D: return VK_IMAGE_TYPE_1D;
                case ImageType::IMAGE_2D: return VK_IMAGE_TYPE_2D;
                case ImageType::IMAGE_3D: return VK_IMAGE_TYPE_3D;
            }
            return VK_IMAGE_TYPE_2D;
        }

        VkImageViewType toVkImageViewType(ImageType type, uint32_t arrayLayers) {
            switch (type) {
                case ImageType::IMAGE_1D:
                    return (arrayLayers > 1) ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
                case ImageType::IMAGE_2D:
                    return (arrayLayers > 1) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
                case ImageType::IMAGE_3D:
                    return VK_IMAGE_VIEW_TYPE_3D;
            }
            return VK_IMAGE_VIEW_TYPE_2D;
        }

        uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                                VkMemoryPropertyFlags properties) {
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

            for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
                if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) ==
                    properties) {
                    return i;
                }
            }

            LAVAVK_ERROR("[LavaVK ERROR] Failed to find suitable memory type for image allocation.");
        }

        VkImageView createImageView(
            VkDevice device,
            VkImage image,
            VkFormat format,
            VkImageAspectFlags aspectFlags,
            VkImageViewType viewType,
            uint32_t mipLevels,
            uint32_t arrayLayers) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = viewType;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = mipLevels;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = arrayLayers;

            VkImageView imageView = VK_NULL_HANDLE;
            if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
                LAVAVK_ERROR("[LavaVK ERROR] Failed to create image view.");
            }

            return imageView;
        }
    } // namespace

    Image::Image(Device &device, const ImageCreateInfo &info)
        : m_device(&device),
          m_extent(info.extent),
          m_format(info.format),
          m_ownsImage(true) {
        // 1. Create Image Handle
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = toVkImageType(info.type);
        imageInfo.extent = info.extent;
        imageInfo.mipLevels = info.mipLevels;
        imageInfo.arrayLayers = info.arrayLayers;
        imageInfo.format = info.format;
        imageInfo.tiling = info.tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = static_cast<VkImageUsageFlags>(info.usage);
        imageInfo.samples = info.samples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(m_device->native(), &imageInfo, nullptr, &m_image) != VK_SUCCESS) {
            LAVAVK_ERROR("[LavaVK ERROR] Failed to create image.");
        }

        // 2. Allocate and Bind Memory
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_device->native(), m_image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(
            m_device->physical(),
            memRequirements.memoryTypeBits,
            info.memory);

        if (vkAllocateMemory(m_device->native(), &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
            vkDestroyImage(m_device->native(), m_image, nullptr);
            m_image = VK_NULL_HANDLE;
            LAVAVK_ERROR("[LavaVK ERROR] Failed to allocate image memory.");
        }

        vkBindImageMemory(m_device->native(), m_image, m_memory, 0);

        // 3. Create Image View
        VkImageViewType viewType = toVkImageViewType(info.type, info.arrayLayers);
        m_view = createImageView(
            m_device->native(),
            m_image,
            m_format,
            info.aspect,
            viewType,
            info.mipLevels,
            info.arrayLayers);
    }

    Image::Image(
        Device &device,
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspect,
        VkExtent3D extent)
        : m_device(&device),
          m_image(image),
          m_format(format),
          m_ownsImage(false),
          m_extent(extent) {
        m_view = createImageView(
            m_device->native(),
            m_image,
            m_format,
            aspect,
            VK_IMAGE_VIEW_TYPE_2D,
            1,
            1);
    }

    Image::~Image() {
        if (m_device != nullptr) {
            if (m_view != VK_NULL_HANDLE) {
                vkDestroyImageView(m_device->native(), m_view, nullptr);
            }

            if (m_ownsImage) {
                if (m_image != VK_NULL_HANDLE) {
                    vkDestroyImage(m_device->native(), m_image, nullptr);
                }
                if (m_memory != VK_NULL_HANDLE) {
                    vkFreeMemory(m_device->native(), m_memory, nullptr);
                }
            }
        }
    }

    Image::Image(Image &&other) noexcept
        : m_device(other.m_device),
          m_image(other.m_image),
          m_view(other.m_view),
          m_memory(other.m_memory),
          m_extent(other.m_extent),
          m_format(other.m_format),
          m_ownsImage(other.m_ownsImage) {
        other.m_device = nullptr;
        other.m_image = VK_NULL_HANDLE;
        other.m_view = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
        other.m_ownsImage = false;
    }

    Image &Image::operator=(Image &&other) noexcept {
        if (this != &other) {
            if (m_device != nullptr) {
                if (m_view != VK_NULL_HANDLE) {
                    vkDestroyImageView(m_device->native(), m_view, nullptr);
                }
                if (m_ownsImage) {
                    if (m_image != VK_NULL_HANDLE) {
                        vkDestroyImage(m_device->native(), m_image, nullptr);
                    }
                    if (m_memory != VK_NULL_HANDLE) {
                        vkFreeMemory(m_device->native(), m_memory, nullptr);
                    }
                }
            }

            m_device = other.m_device;
            m_image = other.m_image;
            m_view = other.m_view;
            m_memory = other.m_memory;
            m_extent = other.m_extent;
            m_format = other.m_format;
            m_ownsImage = other.m_ownsImage;

            other.m_device = nullptr;
            other.m_image = VK_NULL_HANDLE;
            other.m_view = VK_NULL_HANDLE;
            other.m_memory = VK_NULL_HANDLE;
            other.m_ownsImage = false;
        }
        return *this;
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
            throw std::runtime_error("LavaVK::Buffer - Failed to create VkBuffer!");
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
            throw std::runtime_error("LavaVK::Buffer - Failed to allocate buffer memory!");
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
                throw std::runtime_error("LavaVK::Buffer - Failed to map memory!");
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
