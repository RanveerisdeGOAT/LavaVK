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
        IMAGE_1D, /**< A one-dimensional image (width only). */
        IMAGE_2D, /**< A two-dimensional image (width and height). */
        IMAGE_3D  /**< A three-dimensional image (width, height, and depth). */
    };

    /**
     * @brief Specifies how an image will be used by the GPU, mirroring `VkImageUsageFlagBits`.
     * @details Multiple usage flags may be combined with the bitwise OR operator.
     */
    enum class ImageUsage : VkImageUsageFlags {
        NONE = 0, /**< No usage specified. */
        TRANSFER_SRC = VK_IMAGE_USAGE_TRANSFER_SRC_BIT, /**< Usable as the source of a transfer (copy/blit) command. */
        TRANSFER_DST = VK_IMAGE_USAGE_TRANSFER_DST_BIT, /**< Usable as the destination of a transfer (copy/blit) command. */
        SAMPLED = VK_IMAGE_USAGE_SAMPLED_BIT, /**< Usable as a sampled image bound to a shader (e.g. via a combined image sampler). */
        STORAGE = VK_IMAGE_USAGE_STORAGE_BIT, /**< Usable as a storage image for read/write shader access. */
        COLOR_ATTACHMENT = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, /**< Usable as a color attachment in a framebuffer. */
        DEPTH_ATTACHMENT = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, /**< Usable as a depth/stencil attachment in a framebuffer. */
        TRANSIENT = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, /**< Hints that the attachment's memory should be lazily allocated, for transient render targets. */
        INPUT_ATTACHMENT = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT /**< Usable as an input attachment, read by a shader within the same render pass. */
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
     * @brief Image layout, mirroring `VkImageLayout`.
     * @details Describes how an image's memory is currently arranged/optimized
     * for a particular kind of GPU access. Layout transitions are recorded
     * via `CommandBuffer::pipelineBarrier()`; passing the correct old/new
     * layout is required for Vulkan validation and for the driver to
     * perform any necessary memory reformatting.
     */
    enum class ImageLayout {
        UNDEFINED = VK_IMAGE_LAYOUT_UNDEFINED, /**< Contents are undefined; valid only as the "old" layout when contents don't need preserving. */
        GENERAL = VK_IMAGE_LAYOUT_GENERAL, /**< Supports all types of device access, at a possible performance cost. */
        COLOR_ATTACHMENT_OPTIMAL = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, /**< Optimal for use as a color attachment. */
        DEPTH_STENCIL_ATTACHMENT_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, /**< Optimal for use as a read/write depth/stencil attachment. */
        DEPTH_STENCIL_READ_ONLY_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, /**< Optimal for read-only depth/stencil attachment and shader access. */
        SHADER_READ_ONLY_OPTIMAL = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, /**< Optimal for read-only shader access (sampled or input attachment). */
        TRANSFER_SRC_OPTIMAL = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, /**< Optimal as the source of a transfer command. */
        TRANSFER_DST_OPTIMAL = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, /**< Optimal as the destination of a transfer command. */
        PREINITIALIZED = VK_IMAGE_LAYOUT_PREINITIALIZED, /**< Contents are preinitialized by the host; valid only as the "old" layout. */
        DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL, /**< Read-only depth, read/write stencil attachment. */
        DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL, /**< Read/write depth, read-only stencil attachment. */
        DEPTH_ATTACHMENT_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, /**< Optimal for use as a read/write depth-only attachment. */
        DEPTH_READ_ONLY_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL, /**< Optimal for read-only depth attachment and shader access. */
        STENCIL_ATTACHMENT_OPTIMAL = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL, /**< Optimal for use as a read/write stencil-only attachment. */
        STENCIL_READ_ONLY_OPTIMAL = VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL, /**< Optimal for read-only stencil attachment and shader access. */
        READ_ONLY_OPTIMAL = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, /**< Optimal for any read-only access (attachment or shader), depth/stencil/color as applicable. */
        ATTACHMENT_OPTIMAL = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, /**< Optimal for use as a read/write attachment, color or depth/stencil as applicable. */
        RENDERING_LOCAL_READ = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ, /**< Optimal for local reads within dynamic rendering (`vkCmdBeginRendering`). */
        PRESENT_SRC_KHR = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, /**< Required layout for presenting a swapchain image to the screen. */
        VIDEO_DECODE_DST_KHR = VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR, /**< Destination of a video decode operation. */
        VIDEO_DECODE_SRC_KHR = VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR, /**< Source of a video decode operation. */
        VIDEO_DECODE_DPB_KHR = VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR, /**< Decoded picture buffer for video decode. */
        SHARED_PRESENT_KHR = VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR, /**< Shared presentable image, readable/writable while presented. */
        FRAGMENT_DENSITY_MAP_OPTIMAL_EXT = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT, /**< Optimal as a fragment density map attachment. */
        FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR, /**< Optimal as a fragment shading rate attachment. */
        VIDEO_ENCODE_DST_KHR = VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR, /**< Destination of a video encode operation. */
        VIDEO_ENCODE_SRC_KHR = VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR, /**< Source of a video encode operation. */
        VIDEO_ENCODE_DPB_KHR = VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR, /**< Decoded picture buffer for video encode. */
        ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT = VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT, /**< Optimal for attachments participating in a feedback loop. */
        TENSOR_ALIASING_ARM = VK_IMAGE_LAYOUT_TENSOR_ALIASING_ARM, /**< ARM-specific layout for tensor memory aliasing. */
        VIDEO_ENCODE_QUANTIZATION_MAP_KHR = VK_IMAGE_LAYOUT_VIDEO_ENCODE_QUANTIZATION_MAP_KHR, /**< Video encode quantization map layout. */
        ZERO_INITIALIZED_EXT = VK_IMAGE_LAYOUT_ZERO_INITIALIZED_EXT, /**< Contents are guaranteed zero-initialized; valid only as the "old" layout. */
        SHADING_RATE_OPTIMAL_NV = VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV, /**< NVIDIA-specific optimal shading rate image layout. */
        RENDERING_LOCAL_READ_KHR = VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR, /**< KHR alias of #RENDERING_LOCAL_READ. */
        DEPTH_ATTACHMENT_OPTIMAL_KHR = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR, /**< KHR alias of #DEPTH_ATTACHMENT_OPTIMAL. */
        DEPTH_READ_ONLY_OPTIMAL_KHR = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL_KHR, /**< KHR alias of #DEPTH_READ_ONLY_OPTIMAL. */
        STENCIL_ATTACHMENT_OPTIMAL_KHR = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR, /**< KHR alias of #STENCIL_ATTACHMENT_OPTIMAL. */
        STENCIL_READ_ONLY_OPTIMAL_KHR = VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL_KHR, /**< KHR alias of #STENCIL_READ_ONLY_OPTIMAL. */
        READ_ONLY_OPTIMAL_KHR = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR, /**< KHR alias of #READ_ONLY_OPTIMAL. */
        ATTACHMENT_OPTIMAL_KHR = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR, /**< KHR alias of #ATTACHMENT_OPTIMAL. */
        VK_IMAGE_LAYOUT_MAX_ENUM = 0x7FFFFFFF /**< Sentinel value forcing the enum's underlying type to 32 bits; not a valid layout. */
    };

    /**
     * @brief Selects which aspect(s) of an image (color, depth, stencil, ...) an
     * operation applies to, mirroring `VkImageAspectFlagBits`.
     * @details Used when creating image views/subresource ranges and when
     * recording layout transitions or copies, so the driver knows which
     * plane(s) of a multi-planar or depth/stencil image to touch.
     */
    enum class ImageAspectFlagBits {
        COLOR_BIT = VK_IMAGE_ASPECT_COLOR_BIT, /**< The color aspect of a color image. */
        DEPTH_BIT = VK_IMAGE_ASPECT_DEPTH_BIT, /**< The depth aspect of a depth/stencil image. */
        STENCIL_BIT = VK_IMAGE_ASPECT_STENCIL_BIT, /**< The stencil aspect of a depth/stencil image. */
        METADATA_BIT = VK_IMAGE_ASPECT_METADATA_BIT, /**< Implementation-specific metadata associated with the image. */
        PLANE_0_BIT = VK_IMAGE_ASPECT_PLANE_0_BIT, /**< Plane 0 of a multi-planar image format. */
        PLANE_1_BIT = VK_IMAGE_ASPECT_PLANE_1_BIT, /**< Plane 1 of a multi-planar image format. */
        PLANE_2_BIT = VK_IMAGE_ASPECT_PLANE_2_BIT, /**< Plane 2 of a multi-planar image format. */
        NONE = VK_IMAGE_ASPECT_NONE, /**< No aspect selected. */
        MEMORY_PLANE_0_BIT_EXT = VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT, /**< Memory plane 0, for disjoint multi-planar images. */
        MEMORY_PLANE_1_BIT_EXT = VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT, /**< Memory plane 1, for disjoint multi-planar images. */
        MEMORY_PLANE_2_BIT_EXT = VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT, /**< Memory plane 2, for disjoint multi-planar images. */
        MEMORY_PLANE_3_BIT_EXT = VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT, /**< Memory plane 3, for disjoint multi-planar images. */
        PLANE_0_BIT_KHR = VK_IMAGE_ASPECT_PLANE_0_BIT_KHR, /**< KHR alias of #PLANE_0_BIT. */
        PLANE_1_BIT_KHR = VK_IMAGE_ASPECT_PLANE_1_BIT_KHR, /**< KHR alias of #PLANE_1_BIT. */
        PLANE_2_BIT_KHR = VK_IMAGE_ASPECT_PLANE_2_BIT_KHR, /**< KHR alias of #PLANE_2_BIT. */
        NONE_KHR = VK_IMAGE_ASPECT_NONE_KHR, /**< KHR alias of #NONE. */
        FLAG_BITS_MAX_ENUM = VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM /**< Sentinel value forcing the enum's underlying type to 32 bits; not a valid aspect. */
    };

    /**
     * @brief Combines two ImageAspectFlagBits flags.
     * @param lhs First set of aspect flags.
     * @param rhs Second set of aspect flags.
     * @return An #ImageAspectFlagBits containing the bitwise OR of @p lhs and @p rhs.
     */
    inline ImageAspectFlagBits operator|(ImageAspectFlagBits lhs, ImageAspectFlagBits rhs) {
        return static_cast<ImageAspectFlagBits>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
    }

    /**
     * @brief Describes the properties of an #Image to be created.
     */
    struct ImageCreateInfo {
        ImageType type = ImageType::IMAGE_2D; /**< Dimensionality of the image. */
        VkExtent3D extent{1, 1, 1}; /**< Width, height, and depth of the image, in texels. */
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM; /**< Native Vulkan pixel format. */
        ImageUsage usage = ImageUsage::SAMPLED; /**< Intended GPU usage (sampled, storage, attachment, transfer, ...). */
        VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT; /**< Aspect mask used when creating the image's default view. */
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT; /**< Multisample sample count. */
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL; /**< Texel tiling arrangement in memory. */
        VkMemoryPropertyFlags memory = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; /**< Required memory properties for the backing allocation. */
        uint32_t mipLevels = 1; /**< Number of mipmap levels. */
        uint32_t arrayLayers = 1; /**< Number of array layers. */
    };

    /**
     * @brief Represents a Vulkan image (`VkImage`) together with its backing
     * memory and a default `VkImageView`.
     *
     * @details
     * `Image` supports two modes of construction: it can allocate and own a
     * brand-new `VkImage` and `VkDeviceMemory` from an #ImageCreateInfo, or
     * it can wrap an externally-owned `VkImage` (for example, a swapchain
     * image) without taking ownership of the underlying handle — see
     * #ownsImage(). In both cases a `VkImageView` covering the whole image
     * is created and owned by this object. `Image` is move-only RAII; the
     * destructor destroys the view, and additionally the image and memory
     * if they are owned.
     */
    class Image {
    public:
        /**
         * @brief Constructs an empty image handle.
         * @details Leaves the image in an unowned state (`image()` returns
         * `VK_NULL_HANDLE`) until move-assigned from a constructed `Image`.
         */
        Image() = default;

        /**
         * @brief Allocates a new Vulkan image, backing memory, and default view.
         * @param device Logical device used to create the image.
         * @param info Image creation parameters (type, extent, format, usage, ...).
         * @throw std::runtime_error If image creation, memory allocation/binding,
         * or view creation fails.
         */
        Image(Device &device, const ImageCreateInfo &info);

        /**
         * @brief Wraps an externally-owned `VkImage` (e.g. a swapchain image)
         * and creates a view for it.
         * @details The resulting `Image` does not own @p image: the
         * destructor destroys only the created view, leaving @p image's
         * lifetime managed by its original owner. See #ownsImage().
         * @param device Logical device used to create the image view.
         * @param image Existing, externally-owned `VkImage` handle to wrap.
         * @param format Format of @p image, used to create its view.
         * @param aspect Aspect mask used to create @p image's view.
         * @param extent Dimensions of @p image.
         * @throw std::runtime_error If view creation fails.
         */
        Image(Device &device, VkImage image, VkFormat format, VkImageAspectFlags aspect, VkExtent3D extent);

        /**
         * @brief Destroys the image view, and the image and its memory if owned.
         * @details A moved-from `Image` is a no-op. See #ownsImage().
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
         * @param other The image being moved from; left in an empty, destructible state.
         * @return Reference to this image.
         */
        Image &operator=(Image &&other) noexcept;

        /**
         * @brief Returns the native Vulkan image handle.
         * @return The underlying `VkImage` handle, or `VK_NULL_HANDLE` if unconstructed/moved-from.
         */
        [[nodiscard]] VkImage image() const { return m_image; }

        /**
         * @brief Returns the native Vulkan image view handle covering the whole image.
         * @return The underlying `VkImageView` handle.
         */
        [[nodiscard]] VkImageView view() const { return m_view; }

        /**
         * @brief Returns the pixel format of the image.
         * @return The `VkFormat` this image was created with.
         */
        [[nodiscard]] VkFormat format() const { return m_format; }

        /**
         * @brief Returns the dimensions of the image.
         * @return The `VkExtent3D` (width, height, depth) this image was created with.
         */
        [[nodiscard]] VkExtent3D extent() const { return m_extent; }

        /**
         * @brief Returns whether this `Image` owns its underlying `VkImage`/`VkDeviceMemory`.
         * @details True for images allocated via the #ImageCreateInfo
         * constructor; false for images wrapping an externally-owned handle
         * (e.g. constructed from a swapchain image), in which case the
         * destructor will not destroy the `VkImage` itself.
         * @return True if the destructor will destroy the underlying `VkImage` and its memory.
         */
        [[nodiscard]] bool ownsImage() const { return m_ownsImage; }

        ImageLayout layout() const
        {
            return m_layout;
        }

        void setLayout(ImageLayout layout)
        {
            m_layout = layout;
        }

    private:
        Device *m_device = nullptr;
        VkImage m_image = VK_NULL_HANDLE;
        VkImageView m_view = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;
        VkExtent3D m_extent{};
        VkFormat m_format = VK_FORMAT_UNDEFINED;
        bool m_ownsImage = true;
        ImageLayout m_layout = ImageLayout::UNDEFINED;
    };

    /** @brief Border color used by a sampler for `CLAMP_TO_BORDER` address modes, mirroring `VkBorderColor`. */
    enum class BorderColor {
        FLOAT_TRANSPARENT_BLACK = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK, /**< Transparent black, floating-point formats: (0, 0, 0, 0). */
        INT_TRANSPARENT_BLACK = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK, /**< Transparent black, integer formats: (0, 0, 0, 0). */
        FLOAT_OPAQUE_BLACK = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK, /**< Opaque black, floating-point formats: (0, 0, 0, 1). */
        INT_OPAQUE_BLACK = VK_BORDER_COLOR_INT_OPAQUE_BLACK, /**< Opaque black, integer formats: (0, 0, 0, 1). */
        FLOAT_OPAQUE_WHITE = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, /**< Opaque white, floating-point formats: (1, 1, 1, 1). */
        INT_OPAQUE_WHITE = VK_BORDER_COLOR_INT_OPAQUE_WHITE, /**< Opaque white, integer formats: (1, 1, 1, 1). */
        FLOAT_CUSTOM_EXT = VK_BORDER_COLOR_FLOAT_CUSTOM_EXT, /**< Application-supplied floating-point border color. */
        INT_CUSTOM_EXT = VK_BORDER_COLOR_INT_CUSTOM_EXT, /**< Application-supplied integer border color. */
        MAX_ENUM = VK_BORDER_COLOR_MAX_ENUM /**< Sentinel value forcing the enum's underlying type to 32 bits; not a valid border color. */
    };

    /** @brief Texel filtering mode used for sampler magnification/minification, mirroring `VkFilter`. */
    enum class Filter {
        NEAREST = VK_FILTER_NEAREST, /**< Nearest-neighbor filtering. */
        LINEAR = VK_FILTER_LINEAR, /**< Linear (bilinear/trilinear) filtering. */
        CUBIC_EXT = VK_FILTER_CUBIC_EXT, /**< Cubic filtering (EXT extension). */
        CUBIC_IMG = VK_FILTER_CUBIC_IMG, /**< Cubic filtering (IMG extension). */
        MAX_ENUM = VK_FILTER_MAX_ENUM, /**< Sentinel value forcing the enum's underlying type to 32 bits; not a valid filter. */
    };

    /** @brief Behavior of a sampler when texture coordinates fall outside `[0, 1)`, mirroring `VkSamplerAddressMode`. */
    enum class SamplerAddressMode {
        REPEAT = VK_SAMPLER_ADDRESS_MODE_REPEAT, /**< The texture repeats/tiles. */
        MIRRORED_REPEAT = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, /**< The texture repeats, mirroring on each repeat. */
        CLAMP_TO_EDGE = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, /**< Coordinates are clamped to the edge texels. */
        CLAMP_TO_BORDER = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, /**< Coordinates outside the range sample a fixed #BorderColor. */
        MIRROR_CLAMP_TO_EDGE = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE, /**< Mirrors once, then clamps to the edge. */
        MIRROR_CLAMP_TO_EDGE_KHR = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE_KHR, /**< KHR alias of #MIRROR_CLAMP_TO_EDGE. */
        MAX_ENUM = VK_SAMPLER_ADDRESS_MODE_MAX_ENUM /**< Sentinel value forcing the enum's underlying type to 32 bits; not a valid address mode. */
    };

    /**
     * @brief Describes the properties of a `VkSampler` to be created for a #Texture.
     */
    struct TextureSamplerCreateInfo {
        Filter magFilter = Filter::LINEAR; /**< Filtering applied when magnifying (texel smaller than pixel). */
        Filter minFilter = Filter::LINEAR; /**< Filtering applied when minifying (texel larger than pixel). */
        SamplerAddressMode addressModeU = SamplerAddressMode::REPEAT; /**< Address mode for the U (horizontal) texture coordinate. */
        SamplerAddressMode addressModeV = SamplerAddressMode::REPEAT; /**< Address mode for the V (vertical) texture coordinate. */
        SamplerAddressMode addressModeW = SamplerAddressMode::REPEAT; /**< Address mode for the W (depth) texture coordinate. */
        bool enableAnisotropy = false; /**< Enables anisotropic filtering. */
        float maxAnisotropy = 1.0f; /**< Maximum anisotropy clamp used when #enableAnisotropy is true. */
    };

    /**
     * @brief Creation parameters for empty GPU textures (e.g., storage images, render targets).
     */
    struct TextureCreateInfo {
        uint32_t width = 0; /**< Texture width, in texels. */
        uint32_t height = 0; /**< Texture height, in texels. */
        Format format = Format(ChannelOrder::RGBA, BitDepth::B8, NumericType::Unorm); /**< Pixel format of the texture. */
        ImageUsage usage = ImageUsage::SAMPLED; /**< Intended GPU usage of the underlying image. */
        TextureSamplerCreateInfo samplerInfo{}; /**< Sampler parameters used to sample the texture in shaders. */
    };

    /**
     * @brief Represents a GPU texture: an #Image paired with a `VkSampler`.
     *
     * @details
     * `Texture` combines the two resources most shader sampling operations
     * need together: an #Image (with its view) and a `VkSampler` describing
     * how that image is filtered and addressed. It supports two creation
     * paths: allocating an empty GPU-only image (e.g. a storage image or
     * render target) from a #TextureCreateInfo, or loading image data from
     * disk and uploading it to a new sampled image. `Texture` is move-only
     * RAII: the sampler is created in the constructor and destroyed, along
     * with the owned #Image, in the destructor.
     *
     * Example, loading a texture from disk:
     * @code
     * LavaVK::Texture texture(device, "assets/textures/brick.png");
     * @endcode
     */
    class Texture {
    public:
        /**
         * @brief Constructs an empty texture GPU resource (e.g., storage image or compute target).
         * @param device Logical device used to create the underlying image and sampler.
         * @param info Texture creation parameters (dimensions, format, usage, sampler settings).
         * @throw std::runtime_error If image or sampler creation fails.
         */
        Texture(Device &device, const TextureCreateInfo &info);

        /**
         * @brief Loads an image file from disk and uploads it to the GPU as a sampled texture.
         * @param device Logical device used to create the underlying image and sampler.
         * @param path Filesystem path to the image file to load.
         * @param samplerInfo Sampler parameters used to sample the texture in shaders.
         * @throw std::runtime_error If the file cannot be loaded, or if image/sampler creation or upload fails.
         */
        Texture(
            Device &device,
            const std::filesystem::path &path,
            const TextureSamplerCreateInfo &samplerInfo = {}
        );

        /**
         * @brief Destroys the sampler and the owned #Image.
         * @details A moved-from `Texture` is a no-op.
         */
        ~Texture();

        Texture(const Texture &) = delete;
        Texture &operator=(const Texture &) = delete;

        /**
         * @brief Transfers ownership from another Texture.
         * @param other The texture being moved from; left in an empty, destructible state.
         */
        Texture(Texture&& other) noexcept;

        /**
         * @brief Transfers ownership from another Texture.
         * @param other The texture being moved from; left in an empty, destructible state.
         * @return Reference to this texture.
         */
        Texture& operator=(Texture&& other) noexcept;

        /**
         * @brief Returns the texture's underlying image.
         * @return Const reference to the #Image backing this texture.
         */
        [[nodiscard]] const Image &image() const { return m_image; }

        /**
         * @brief Returns the texture's underlying image.
         * @return Reference to the #Image backing this texture.
         */
        [[nodiscard]] Image &image() { return m_image; }

        /**
         * @brief Returns the native Vulkan sampler handle.
         * @return The underlying `VkSampler` handle, or `VK_NULL_HANDLE` if unconstructed/moved-from.
         */
        [[nodiscard]] VkSampler sampler() const { return m_sampler; }

    private:
        /**
         * @brief Creates the `VkSampler` from the given sampler parameters.
         * @param samplerInfo Sampler parameters (filtering, address modes, anisotropy).
         * @throw std::runtime_error If `vkCreateSampler` fails.
         */
        void createSampler(const TextureSamplerCreateInfo &samplerInfo);

        Device &m_device;
        Image m_image{};
        VkSampler m_sampler = VK_NULL_HANDLE;
    };



}

#endif // LAVAVK_TEXTURE_HPP