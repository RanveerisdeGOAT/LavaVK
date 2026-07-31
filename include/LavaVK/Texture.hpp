//
// Created by paikr on 7/30/2026.
//

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
        IMAGE_1D, /**< One-dimensional image. */
        IMAGE_2D, /**< Two-dimensional image (the common case: textures, render targets). */
        IMAGE_3D /**< Three-dimensional (volumetric) image. */
    };

    /**
     * @brief Specifies how an image will be used by the GPU.
     *
     * Multiple usage flags may be combined with the bitwise OR operator.
     */
    enum class ImageUsage : VkImageUsageFlags {
        NONE = 0, /**< No usage specified. */
        TRANSFER_SRC = VK_IMAGE_USAGE_TRANSFER_SRC_BIT, /**< Usable as the source of a transfer (copy/blit) command. */
        TRANSFER_DST = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        /**< Usable as the destination of a transfer (copy/blit) command. */
        SAMPLED = VK_IMAGE_USAGE_SAMPLED_BIT, /**< Usable as a sampled texture in a shader. */
        STORAGE = VK_IMAGE_USAGE_STORAGE_BIT, /**< Usable as a storage image for read/write shader access. */
        COLOR_ATTACHMENT = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, /**< Usable as a color attachment in a framebuffer. */
        DEPTH_ATTACHMENT = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        /**< Usable as a depth/stencil attachment in a framebuffer. */
        TRANSIENT = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
        /**< Hints the implementation this attachment's contents need not persist. */
        INPUT_ATTACHMENT = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
        /**< Usable as an input attachment, read by a shader within the same subpass. */
    };

    /**
     * @brief Combines two ImageUsage flags.
     * @param a First set of usage flags.
     * @param b Second set of usage flags.
     * @return An #ImageUsage containing the bitwise OR of @p a and @p b.
     */
    inline ImageUsage operator|(ImageUsage a, ImageUsage b) {
        return static_cast<ImageUsage>(
            static_cast<VkImageUsageFlags>(a) |
            static_cast<VkImageUsageFlags>(b));
    }

    /**
     * @brief Describes the properties of an image to be created.
     */
    struct ImageCreateInfo {
        /**
         * @brief Image dimensionality.
         */
        ImageType type = ImageType::IMAGE_2D;

        /**
         * @brief Image width, height, and depth.
         */
        VkExtent3D extent
        {
            1,
            1,
            1
        };

        /**
         * @brief Pixel format of the image.
         */
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

        /**
         * @brief Intended usage of the image.
         */
        ImageUsage usage = ImageUsage::SAMPLED;

        /**
         * @brief Aspect(s) accessible through the image view.
         */
        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;

        /**
         * @brief Number of MSAA samples.
         */
        VkSampleCountFlagBits samples =
                VK_SAMPLE_COUNT_1_BIT;

        /**
         * @brief Image tiling mode.
         */
        VkImageTiling tiling =
                VK_IMAGE_TILING_OPTIMAL;

        /**
         * @brief Memory properties required for the image allocation.
         */
        VkMemoryPropertyFlags memory =
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        /**
         * @brief Number of mip levels.
         */
        uint32_t mipLevels = 1;

        /**
         * @brief Number of array layers.
         */
        uint32_t arrayLayers = 1;
    };

    /**
     * @brief Represents a Vulkan image and its associated image view.
     *
     * @details
     * An Image may either own its underlying VkImage and allocated memory,
     * or simply wrap an externally created image (such as a swapchain image).
     * Owning images allocate device memory and create both the `VkImage` and
     * `VkImageView` in the constructor, tearing both down in the destructor.
     * Wrapped (non-owning) images — for example swapchain images, which are
     * owned and destroyed by the swapchain itself — only create the
     * `VkImageView`, leaving the underlying `VkImage` and its memory alone.
     * `Image` is move-only RAII; copying is disabled.
     *
     * Example, creating an owned depth attachment:
     * @code
     * LavaVK::Image depthImage(device, {
     *     .extent = { WIDTH, HEIGHT, 1 },
     *     .format = VK_FORMAT_D32_SFLOAT,
     *     .usage = LavaVK::ImageUsage::DEPTH_ATTACHMENT,
     *     .aspect = VK_IMAGE_ASPECT_DEPTH_BIT
     * });
     * @endcode
     *
     * Example, wrapping a swapchain-owned image:
     * @code
     * LavaVK::Image swapchainImage(device, rawSwapchainImage, colorFormat,
     *                               VK_IMAGE_ASPECT_COLOR_BIT, { WIDTH, HEIGHT, 1 });
     * @endcode
     */
    class Image {
    public:
        /**
         * @brief Constructs an empty image.
         * @details Leaves the image in an unowned state (`image()` and
         * `view()` return `VK_NULL_HANDLE`) until move-assigned from a
         * constructed `Image`.
         */
        Image() = default;

        /**
         * @brief Creates a new Vulkan image.
         *
         * @details Allocates and binds device memory satisfying
         * @p info.memory, then creates an image view covering all mip
         * levels and array layers described by @p info. The resulting
         * `Image` owns both the `VkImage` and its memory.
         *
         * @param device Logical device used to create the image.
         * @param info Image creation parameters.
         * @throw std::runtime_error If image creation, memory allocation, or image view creation fails.
         */
        Image(
            Device &device,
            const ImageCreateInfo &info);

        /**
         * @brief Wraps an existing Vulkan image.
         *
         * No image memory is allocated and the image will not be destroyed
         * by this object.
         *
         * @param device Logical device owning the image.
         * @param image Existing Vulkan image.
         * @param format Image format.
         * @param aspect Image aspect used for the created image view.
         * @param extent Image dimensions.
         * @throw std::runtime_error If image view creation fails.
         */
        Image(
            Device &device,
            VkImage image,
            VkFormat format,
            VkImageAspectFlags aspect,
            VkExtent3D extent);

        /**
         * @brief Destroys the image view and owned image resources.
         * @details Always destroys the `VkImageView`. Additionally destroys
         * the `VkImage` and frees its `VkDeviceMemory` only if #ownsImage()
         * is `true`; a moved-from `Image` is a no-op.
         */
        ~Image();

        Image(const Image &) = delete;

        Image &operator=(const Image &) = delete;

        /**
         * @brief Transfers ownership from another Image.
         * @param other The image being moved from; left in an empty, destructible state.
         */
        Image(Image &&other) noexcept;

        /**
         * @brief Transfers ownership from another Image.
         *
         * @param other The image being moved from; left in an empty, destructible state.
         * @return Reference to this image.
         */
        Image &operator=(Image &&other) noexcept;

        /**
         * @brief Returns the underlying Vulkan image.
         * @return The native `VkImage` handle, or `VK_NULL_HANDLE` if unconstructed/moved-from.
         */
        [[nodiscard]]
        VkImage image() const {
            return m_image;
        }

        /**
         * @brief Returns the associated image view.
         * @return The native `VkImageView` handle, or `VK_NULL_HANDLE` if unconstructed/moved-from.
         */
        [[nodiscard]]
        VkImageView view() const {
            return m_view;
        }

        /**
         * @brief Returns the image format.
         * @return The `VkFormat` this image was created or wrapped with.
         */
        [[nodiscard]]
        VkFormat format() const {
            return m_format;
        }

        /**
         * @brief Returns the image dimensions.
         * @return The image's width, height, and depth as a `VkExtent3D`.
         */
        [[nodiscard]]
        VkExtent3D extent() const {
            return m_extent;
        }

        /**
         * @brief Returns whether this object owns the underlying image.
         *
         * Images wrapping swapchain images return false.
         *
         * @return `true` if this `Image` allocated and will destroy the
         * underlying `VkImage`/memory itself; `false` if it merely wraps an
         * externally owned image.
         */
        [[nodiscard]]
        bool ownsImage() const {
            return m_ownsImage;
        }

    private:
        Device *m_device = nullptr;

        VkImage m_image = VK_NULL_HANDLE;
        VkImageView m_view = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;

        VkExtent3D m_extent{};
        VkFormat m_format = VK_FORMAT_UNDEFINED;

        bool m_ownsImage = true;
    };

    /**
     * @brief Border color used when a sampler's address mode clamps to a border,
     * mirroring `VkBorderColor`.
     * @details Only relevant when a sampler's address mode is
     * #SamplerAddressMode::CLAMP_TO_BORDER; ignored otherwise.
     */
    enum class BorderColor {
        FLOAT_TRANSPARENT_BLACK = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        /**< Transparent black, floating-point format: (0, 0, 0, 0). */
        INT_TRANSPARENT_BLACK = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK,
        /**< Transparent black, integer format: (0, 0, 0, 0). */
        FLOAT_OPAQUE_BLACK = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        /**< Opaque black, floating-point format: (0, 0, 0, 1). */
        INT_OPAQUE_BLACK = VK_BORDER_COLOR_INT_OPAQUE_BLACK, /**< Opaque black, integer format: (0, 0, 0, 1). */
        FLOAT_OPAQUE_WHITE = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        /**< Opaque white, floating-point format: (1, 1, 1, 1). */
        INT_OPAQUE_WHITE = VK_BORDER_COLOR_INT_OPAQUE_WHITE, /**< Opaque white, integer format: (1, 1, 1, 1). */
        FLOAT_CUSTOM_EXT = VK_BORDER_COLOR_FLOAT_CUSTOM_EXT,
        /**< Custom floating-point border color (requires `VK_EXT_custom_border_color`). */
        INT_CUSTOM_EXT = VK_BORDER_COLOR_INT_CUSTOM_EXT,
        /**< Custom integer border color (requires `VK_EXT_custom_border_color`). */
        MAX_ENUM = VK_BORDER_COLOR_MAX_ENUM /**< Sentinel value; not a valid border color. */
    };

    /**
     * @brief Texture filtering mode used for magnification/minification, mirroring `VkFilter`.
     */
    enum class Filter {
        NEAREST = VK_FILTER_NEAREST, /**< Nearest-neighbor sampling; no interpolation. */
        LINEAR = VK_FILTER_LINEAR, /**< Bilinear interpolation between neighboring texels. */
        CUBIC_EXT = VK_FILTER_CUBIC_EXT, /**< Cubic filtering (requires `VK_EXT_filter_cubic`). */
        CUBIC_IMG = VK_FILTER_CUBIC_IMG, /**< Cubic filtering (requires `VK_IMG_filter_cubic`). */
        MAX_ENUM = VK_FILTER_MAX_ENUM, /**< Sentinel value; not a valid filter. */
    };

    /**
     * @brief Sampler addressing behavior for texture coordinates outside the [0, 1) range,
     * mirroring `VkSamplerAddressMode`.
     */
    enum class SamplerAddressMode {
        REPEAT = VK_SAMPLER_ADDRESS_MODE_REPEAT, /**< The texture repeats, tiling infinitely. */
        MIRRORED_REPEAT = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
        /**< The texture repeats, mirroring on each repeat. */
        CLAMP_TO_EDGE = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, /**< Coordinates are clamped to the edge texel. */
        CLAMP_TO_BORDER = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        /**< Coordinates outside range sample a fixed border color. */
        MIRROR_CLAMP_TO_EDGE = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE, /**< Mirrors once, then clamps to edge. */
        MIRROR_CLAMP_TO_EDGE_KHR = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE_KHR,
        /**< KHR alias of #MIRROR_CLAMP_TO_EDGE. */
        MAX_ENUM = VK_SAMPLER_ADDRESS_MODE_MAX_ENUM /**< Sentinel value; not a valid address mode. */
    };

    /**
     * @brief Describes the sampler settings used to read a #Texture in a shader.
     */
    struct TextureSamplerCreateInfo {
        /** @brief Filtering mode used when a texel maps to more than one pixel (magnification). */
        Filter magFilter = Filter::LINEAR;
        /** @brief Filtering mode used when a texel maps to less than one pixel (minification). */
        Filter minFilter = Filter::LINEAR;
        /** @brief Address mode applied to the U (horizontal) texture coordinate. */
        SamplerAddressMode addressModeU = SamplerAddressMode::REPEAT;
        /** @brief Address mode applied to the V (vertical) texture coordinate. */
        SamplerAddressMode addressModeV = SamplerAddressMode::REPEAT;
        /** @brief Address mode applied to the W (depth) texture coordinate. */
        SamplerAddressMode addressModeW = SamplerAddressMode::REPEAT;
        /** @brief Whether anisotropic filtering is enabled. */
        bool enableAnisotropy = false;
        /** @brief Maximum anisotropy level to apply when #enableAnisotropy is `true`. */
        float maxAnisotropy = 1.0f;
    };

    /**
     * @brief Loads an image file from disk into a GPU-resident, shader-sampleable texture.
     *
     * @details
     * `Texture` combines an #Image (created with `ImageUsage::TRANSFER_DST |
     * ImageUsage::SAMPLED`) with a `VkSampler`. On construction it loads
     * pixel data from @p path via stb_image, uploads it to the GPU through a
     * temporary staging buffer and a one-time command buffer (transitioning
     * the image from `VK_IMAGE_LAYOUT_UNDEFINED` to
     * `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` and finally to
     * `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL`), and creates a `VkSampler`
     * configured from @p samplerInfo. The image is always loaded and
     * uploaded as 4-channel (RGBA) `VK_FORMAT_R8G8B8A8_SRGB` data,
     * regardless of the source file's original channel count.
     *
     * `Texture` is neither copyable nor movable; it must be constructed
     * in place (e.g. via `std::unique_ptr<Texture>` or emplaced directly)
     * and its lifetime must not outlive the owning #Device.
     *
     * @note The upload path submits and waits on a temporary, transient
     * command pool synchronously, so construction blocks until the GPU
     * finishes uploading the texture.
     *
     * @warning The sampler's border color is currently always created as
     * `VK_BORDER_COLOR_INT_OPAQUE_BLACK`, regardless of any #BorderColor
     * value; there is currently no way to customize it through
     * #TextureSamplerCreateInfo.
     *
     * Example:
     * @code
     * LavaVK::Texture texture(device, "assets/brick_wall.png", {
     *     .magFilter = LavaVK::Filter::LINEAR,
     *     .minFilter = LavaVK::Filter::LINEAR,
     *     .addressModeU = LavaVK::SamplerAddressMode::REPEAT,
     *     .addressModeV = LavaVK::SamplerAddressMode::REPEAT,
     * });
     *
     * // Later, when writing a descriptor set:
     * VkDescriptorImageInfo imageInfo{};
     * imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
     * imageInfo.imageView = texture.image().view();
     * imageInfo.sampler = texture.sampler();
     * @endcode
     */
    class Texture {
    public:
        /**
         * @brief Loads an image from disk and uploads it to the GPU as a sampled texture.
         * @param device Logical device used to create the image, staging buffer, and sampler.
         * @param path Filesystem path to the image file to load (any format supported by stb_image).
         * @param samplerInfo Sampler configuration (filtering, addressing, anisotropy) to create alongside the image.
         * @throw std::runtime_error If the image file cannot be loaded, if
         * command pool/buffer creation fails, or if sampler creation fails.
         */
        Texture(
            Device &device,
            const std::filesystem::path &path,
            const TextureSamplerCreateInfo &samplerInfo = {}
        );

        /**
         * @brief Destroys the texture's sampler.
         * @details The underlying #Image is destroyed automatically as a
         * member of this object when its own destructor runs.
         */
        ~Texture();

        Texture(const Texture &) = delete;

        Texture &operator=(const Texture &) = delete;

        Texture(Texture&& other) noexcept
    : m_device(other.m_device),
      m_image(std::move(other.m_image)),
      memory(other.memory),
      m_sampler(other.m_sampler) {

            other.memory = VK_NULL_HANDLE;
            other.m_sampler = VK_NULL_HANDLE;
        }

        Texture& operator=(Texture&&) = delete;

        /**
         * @brief Returns the GPU image backing this texture.
         * @return Mutable reference to the underlying #Image, so its
         * `VkImageView` can be bound in a descriptor write.
         */
        Image &image() const;

        /**
         * @brief Returns the sampler created for this texture.
         * @return The native `VkSampler` handle used to read this texture in a shader.
         */
        VkSampler sampler() const;

    private:
        Device &m_device;
        Image m_image{};
        VkDeviceMemory memory{};
        VkSampler m_sampler{};
    };
}
#endif //LAVAVK_TEXTURE_HPP
