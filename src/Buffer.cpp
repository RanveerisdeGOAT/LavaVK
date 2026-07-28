#include "LavaVK/Buffer.hpp"
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
} // namespace LavaVK
