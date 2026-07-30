#include "LavaVK/Texture.hpp"
#include "LavaVK/Device.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <external/stb_image.h>
#include <stdexcept>

#include "LavaVK/Queue.hpp"

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
    }

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

    Texture::Texture(Device &device, const std::filesystem::path& path,
    const TextureSamplerCreateInfo& customSamplerInfo)
        : m_device_(device)
    {
        // 1. Load pixel data using stb_image
        int width = 0;
        int height = 0;
        int channels = 0;

        stbi_uc* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("Failed to load texture image at: " + path.string());
        }

        const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;

        // 2. Create staging buffer and upload CPU pixels
        Buffer stagingBuffer(
            m_device_,
            BufferCreateInfo{
                .size = imageSize,
                .usage = BufferUsage::TransferSrc,
                .memory = MemoryUsage::CPU_TO_GPU
            }
        );

        stagingBuffer.upload(pixels, imageSize);
        stbi_image_free(pixels);

        // 3. Create destination GPU Image
        VkExtent3D extent{
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            1
        };

        m_image = Image(
            m_device_,
            ImageCreateInfo{
                .type = ImageType::IMAGE_2D,
                .extent = extent,
                .format = VK_FORMAT_R8G8B8A8_SRGB,
                .usage = ImageUsage::TRANSFER_DST | ImageUsage::SAMPLED,
                .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
                .memory = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            }
        );

        // 4. Record and submit commands to copy staging buffer to the GPU Image
        const uint32_t queueFamily = m_device_.getQueueFamily(QueueType::GRAPHICS);

        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(m_device_.native(), queueFamily, 0, &queue);

        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = queueFamily;
        if (vkCreateCommandPool(m_device_.native(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create temporary command pool for texture upload.");
        }

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;
        vkAllocateCommandBuffers(m_device_.native(), &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        // Transition layout: UNDEFINED -> TRANSFER_DST_OPTIMAL
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_image.image();
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        // Copy Buffer -> Image
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = extent;

        vkCmdCopyBufferToImage(
            commandBuffer,
            stagingBuffer.native(),
            m_image.image(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        // Transition layout: TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkDestroyCommandPool(m_device_.native(), commandPool, nullptr);

        // Create Vulkan Sampler
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = static_cast<VkFilter>(customSamplerInfo.magFilter);
        samplerInfo.minFilter = static_cast<VkFilter>(customSamplerInfo.minFilter);
        samplerInfo.addressModeU = static_cast<VkSamplerAddressMode>(customSamplerInfo.addressModeU);
        samplerInfo.addressModeV = static_cast<VkSamplerAddressMode>(customSamplerInfo.addressModeV);
        samplerInfo.addressModeW = static_cast<VkSamplerAddressMode>(customSamplerInfo.addressModeW);

        samplerInfo.anisotropyEnable = customSamplerInfo.enableAnisotropy ? VK_TRUE : VK_FALSE;
        samplerInfo.maxAnisotropy = customSamplerInfo.maxAnisotropy;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(m_device_.native(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture sampler!");
        }

        if (vkCreateSampler(m_device_.native(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture sampler!");
        }
    }

    Texture::~Texture() {
        if (m_sampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_device_.native(), m_sampler, nullptr);
            m_sampler = VK_NULL_HANDLE;
        }
    }

    Image& Texture::image() const {
        return const_cast<Image&>(m_image);
    }

    VkSampler Texture::sampler() const {
        return m_sampler;
    }

} // namespace LavaVK