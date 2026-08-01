// Created by paikr on 7/30/2026.

#ifndef LAVAVK_TEXTURE_HPP
#define LAVAVK_TEXTURE_HPP

#include <filesystem>
#include <vulkan/vulkan.h>

#include "Buffer.hpp"

namespace LavaVK {
    class Device;

    /**
     * @brief Specifies the dimensionality of an image.
     */
    enum class ImageType {
        IMAGE_1D,
        IMAGE_2D,
        IMAGE_3D
    };

    /**
     * @brief Specifies how an image will be used by the GPU.
     */
    enum class ImageUsage : VkImageUsageFlags {
        NONE = 0,
        TRANSFER_SRC = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        TRANSFER_DST = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        SAMPLED = VK_IMAGE_USAGE_SAMPLED_BIT,
        STORAGE = VK_IMAGE_USAGE_STORAGE_BIT,
        COLOR_ATTACHMENT = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        DEPTH_ATTACHMENT = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        TRANSIENT = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
        INPUT_ATTACHMENT = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
    };

    inline ImageUsage operator|(ImageUsage a, ImageUsage b) {
        return static_cast<ImageUsage>(
            static_cast<VkImageUsageFlags>(a) |
            static_cast<VkImageUsageFlags>(b));
    }

    enum class ImageLayout {
        UNDEFINED = VK_IMAGE_LAYOUT_UNDEFINED,
        GENERAL = VK_IMAGE_LAYOUT_GENERAL,
        COLOR_ATTACHMENT_OPTIMAL = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        DEPTH_STENCIL_ATTACHMENT_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        DEPTH_STENCIL_READ_ONLY_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        SHADER_READ_ONLY_OPTIMAL = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        TRANSFER_SRC_OPTIMAL = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        TRANSFER_DST_OPTIMAL = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        PREINITIALIZED = VK_IMAGE_LAYOUT_PREINITIALIZED,
        DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
        DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,
        DEPTH_ATTACHMENT_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        DEPTH_READ_ONLY_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL,
        STENCIL_ATTACHMENT_OPTIMAL = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL,
        STENCIL_READ_ONLY_OPTIMAL = VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL,
        READ_ONLY_OPTIMAL = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
        ATTACHMENT_OPTIMAL = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        RENDERING_LOCAL_READ = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ,
        PRESENT_SRC_KHR = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VIDEO_DECODE_DST_KHR = VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR,
        VIDEO_DECODE_SRC_KHR = VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR,
        VIDEO_DECODE_DPB_KHR = VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR,
        SHARED_PRESENT_KHR = VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR,
        FRAGMENT_DENSITY_MAP_OPTIMAL_EXT = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT,
        FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR,
        VIDEO_ENCODE_DST_KHR = VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR,
        VIDEO_ENCODE_SRC_KHR = VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR,
        VIDEO_ENCODE_DPB_KHR = VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR,
        ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT = VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT,
        TENSOR_ALIASING_ARM = VK_IMAGE_LAYOUT_TENSOR_ALIASING_ARM,
        VIDEO_ENCODE_QUANTIZATION_MAP_KHR = VK_IMAGE_LAYOUT_VIDEO_ENCODE_QUANTIZATION_MAP_KHR,
        ZERO_INITIALIZED_EXT = VK_IMAGE_LAYOUT_ZERO_INITIALIZED_EXT,
        SHADING_RATE_OPTIMAL_NV = VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV,
        RENDERING_LOCAL_READ_KHR = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR,
        DEPTH_ATTACHMENT_OPTIMAL_KHR = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR,
        DEPTH_READ_ONLY_OPTIMAL_KHR = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL_KHR,
        STENCIL_ATTACHMENT_OPTIMAL_KHR = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR,
        STENCIL_READ_ONLY_OPTIMAL_KHR = VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL_KHR,
        READ_ONLY_OPTIMAL_KHR = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR,
        ATTACHMENT_OPTIMAL_KHR = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
        VK_IMAGE_LAYOUT_MAX_ENUM = 0x7FFFFFFF
    };

    enum class ImageAspectFlagBits {
        COLOR_BIT = VK_IMAGE_ASPECT_COLOR_BIT,
        DEPTH_BIT = VK_IMAGE_ASPECT_DEPTH_BIT,
        STENCIL_BIT = VK_IMAGE_ASPECT_STENCIL_BIT,
        METADATA_BIT = VK_IMAGE_ASPECT_METADATA_BIT,
        PLANE_0_BIT = VK_IMAGE_ASPECT_PLANE_0_BIT,
        PLANE_1_BIT = VK_IMAGE_ASPECT_PLANE_1_BIT,
        PLANE_2_BIT = VK_IMAGE_ASPECT_PLANE_2_BIT,
        NONE = VK_IMAGE_ASPECT_NONE,
        MEMORY_PLANE_0_BIT_EXT = VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT,
        MEMORY_PLANE_1_BIT_EXT = VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT,
        MEMORY_PLANE_2_BIT_EXT = VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT,
        MEMORY_PLANE_3_BIT_EXT = VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT,
        PLANE_0_BIT_KHR = VK_IMAGE_ASPECT_PLANE_0_BIT_KHR,
        PLANE_1_BIT_KHR = VK_IMAGE_ASPECT_PLANE_1_BIT_KHR,
        PLANE_2_BIT_KHR = VK_IMAGE_ASPECT_PLANE_2_BIT_KHR,
        NONE_KHR = VK_IMAGE_ASPECT_NONE_KHR,
        FLAG_BITS_MAX_ENUM = VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM
    };

    inline ImageAspectFlagBits operator|(ImageAspectFlagBits lhs, ImageAspectFlagBits rhs) {
        return static_cast<ImageAspectFlagBits>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
    }

    struct ImageCreateInfo {
        ImageType type = ImageType::IMAGE_2D;
        VkExtent3D extent{1, 1, 1};
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        ImageUsage usage = ImageUsage::SAMPLED;
        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkMemoryPropertyFlags memory = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        uint32_t mipLevels = 1;
        uint32_t arrayLayers = 1;
    };

    class Image {
    public:
        Image() = default;
        Image(Device &device, const ImageCreateInfo &info);
        Image(Device &device, VkImage image, VkFormat format, VkImageAspectFlags aspect, VkExtent3D extent);
        ~Image();

        Image(const Image &) = delete;
        Image &operator=(const Image &) = delete;
        Image(Image &&other) noexcept;
        Image &operator=(Image &&other) noexcept;

        [[nodiscard]] VkImage image() const { return m_image; }
        [[nodiscard]] VkImageView view() const { return m_view; }
        [[nodiscard]] VkFormat format() const { return m_format; }
        [[nodiscard]] VkExtent3D extent() const { return m_extent; }
        [[nodiscard]] bool ownsImage() const { return m_ownsImage; }

    private:
        Device *m_device = nullptr;
        VkImage m_image = VK_NULL_HANDLE;
        VkImageView m_view = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;
        VkExtent3D m_extent{};
        VkFormat m_format = VK_FORMAT_UNDEFINED;
        bool m_ownsImage = true;
    };

    enum class BorderColor {
        FLOAT_TRANSPARENT_BLACK = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        INT_TRANSPARENT_BLACK = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK,
        FLOAT_OPAQUE_BLACK = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        INT_OPAQUE_BLACK = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        FLOAT_OPAQUE_WHITE = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        INT_OPAQUE_WHITE = VK_BORDER_COLOR_INT_OPAQUE_WHITE,
        FLOAT_CUSTOM_EXT = VK_BORDER_COLOR_FLOAT_CUSTOM_EXT,
        INT_CUSTOM_EXT = VK_BORDER_COLOR_INT_CUSTOM_EXT,
        MAX_ENUM = VK_BORDER_COLOR_MAX_ENUM
    };

    enum class Filter {
        NEAREST = VK_FILTER_NEAREST,
        LINEAR = VK_FILTER_LINEAR,
        CUBIC_EXT = VK_FILTER_CUBIC_EXT,
        CUBIC_IMG = VK_FILTER_CUBIC_IMG,
        MAX_ENUM = VK_FILTER_MAX_ENUM,
    };

    enum class SamplerAddressMode {
        REPEAT = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        MIRRORED_REPEAT = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        CLAMP_TO_EDGE = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        CLAMP_TO_BORDER = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        MIRROR_CLAMP_TO_EDGE = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE,
        MIRROR_CLAMP_TO_EDGE_KHR = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE_KHR,
        MAX_ENUM = VK_SAMPLER_ADDRESS_MODE_MAX_ENUM
    };

    struct TextureSamplerCreateInfo {
        Filter magFilter = Filter::LINEAR;
        Filter minFilter = Filter::LINEAR;
        SamplerAddressMode addressModeU = SamplerAddressMode::REPEAT;
        SamplerAddressMode addressModeV = SamplerAddressMode::REPEAT;
        SamplerAddressMode addressModeW = SamplerAddressMode::REPEAT;
        bool enableAnisotropy = false;
        float maxAnisotropy = 1.0f;
    };

    /**
     * @brief Creation parameters for empty GPU textures (e.g., storage images, render targets).
     */
    struct TextureCreateInfo {
        uint32_t width = 0;
        uint32_t height = 0;
        Format format = Format(ChannelOrder::RGBA, BitDepth::B8, NumericType::Unorm);
        ImageUsage usage = ImageUsage::SAMPLED;
        TextureSamplerCreateInfo samplerInfo{};
    };

    class Texture {
    public:
        /**
         * @brief Constructs an empty texture GPU resource (e.g., storage image or compute target).
         */
        Texture(Device &device, const TextureCreateInfo &info);

        /**
         * @brief Loads an image file from disk and uploads it to the GPU as a sampled texture.
         */
        Texture(
            Device &device,
            const std::filesystem::path &path,
            const TextureSamplerCreateInfo &samplerInfo = {}
        );

        ~Texture();

        Texture(const Texture &) = delete;
        Texture &operator=(const Texture &) = delete;

        Texture(Texture&& other) noexcept;
        Texture& operator=(Texture&& other) noexcept;

        [[nodiscard]] const Image &image() const { return m_image; }
        [[nodiscard]] Image &image() { return m_image; }
        [[nodiscard]] VkSampler sampler() const { return m_sampler; }

    private:
        void createSampler(const TextureSamplerCreateInfo &samplerInfo);

        Device &m_device;
        Image m_image{};
        VkSampler m_sampler = VK_NULL_HANDLE;
    };
}

#endif // LAVAVK_TEXTURE_HPP