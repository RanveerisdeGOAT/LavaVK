#ifndef LAVAVK_BUFFER_HPP
#define LAVAVK_BUFFER_HPP

#include <vulkan/vulkan.h>
#include "Pipeline.hpp"

namespace LavaVK {

class Device;

/**
 * @brief Specifies the dimensionality of an image.
 */
enum class ImageType
{
    IMAGE_1D,
    IMAGE_2D,
    IMAGE_3D
};

/**
 * @brief Specifies how an image will be used by the GPU.
 *
 * Multiple usage flags may be combined with the bitwise OR operator.
 */
enum class ImageUsage : VkImageUsageFlags
{
    NONE               = 0,
    TRANSFER_SRC       = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    TRANSFER_DST       = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    SAMPLED            = VK_IMAGE_USAGE_SAMPLED_BIT,
    STORAGE            = VK_IMAGE_USAGE_STORAGE_BIT,
    COLOR_ATTACHMENT   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    DEPTH_ATTACHMENT   = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    TRANSIENT          = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
    INPUT_ATTACHMENT   = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
};

/**
 * @brief Combines two ImageUsage flags.
 */
inline ImageUsage operator|(ImageUsage a, ImageUsage b)
{
    return static_cast<ImageUsage>(
        static_cast<VkImageUsageFlags>(a) |
        static_cast<VkImageUsageFlags>(b));
}

/**
 * @brief Describes the properties of an image to be created.
 */
struct ImageCreateInfo
{
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
 * An Image may either own its underlying VkImage and allocated memory,
 * or simply wrap an externally created image (such as a swapchain image).
 */
class Image
{
public:

    /**
     * @brief Constructs an empty image.
     */
    Image() = default;

    /**
     * @brief Creates a new Vulkan image.
     *
     * @param device Logical device used to create the image.
     * @param info Image creation parameters.
     */
    Image(
        Device& device,
        const ImageCreateInfo& info);

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
     */
    Image(
        Device& device,
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspect,
        VkExtent3D extent);

    /**
     * @brief Destroys the image view and owned image resources.
     */
    ~Image();

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    /**
     * @brief Transfers ownership from another Image.
     */
    Image(Image&& other) noexcept;

    /**
     * @brief Transfers ownership from another Image.
     *
     * @return Reference to this image.
     */
    Image& operator=(Image&& other) noexcept;

    /**
     * @brief Returns the underlying Vulkan image.
     */
    [[nodiscard]]
    VkImage image() const
    {
        return m_image;
    }

    /**
     * @brief Returns the associated image view.
     */
    [[nodiscard]]
    VkImageView view() const
    {
        return m_view;
    }

    /**
     * @brief Returns the image format.
     */
    [[nodiscard]]
    VkFormat format() const
    {
        return m_format;
    }

    /**
     * @brief Returns the image dimensions.
     */
    [[nodiscard]]
    VkExtent3D extent() const
    {
        return m_extent;
    }

    /**
     * @brief Returns whether this object owns the underlying image.
     *
     * Images wrapping swapchain images return false.
     */
    [[nodiscard]]
    bool ownsImage() const
    {
        return m_ownsImage;
    }

private:
    Device* m_device = nullptr;

    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_view = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;

    VkExtent3D m_extent{};
    VkFormat m_format = VK_FORMAT_UNDEFINED;

    bool m_ownsImage = true;
};

/**
 * @brief Represents a Vulkan framebuffer.
 *
 * A framebuffer is a collection of image attachments compatible with a
 * specific RenderPass. Attachments typically include a color image,
 * depth image, or additional render targets.
 */
class Framebuffer
{
public:

    /**
     * @brief Creates a framebuffer from a collection of image attachments.
     *
     * @param device Logical device used to create the framebuffer.
     * @param renderPass Compatible render pass.
     * @param attachments Images attached to the framebuffer.
     */
    Framebuffer(
        Device& device,
        RenderPass& renderPass,
        const std::vector<Image*>& attachments);

    /**
     * @brief Destroys the framebuffer.
     */
    ~Framebuffer();

    // Framebuffer(const Framebuffer&) = delete;
    // Framebuffer& operator=(const Framebuffer&) = delete;

    /**
     * @brief Returns the native Vulkan framebuffer.
     */
    [[nodiscard]]
    VkFramebuffer native() const
    {
        return m_framebuffer;
    }

    /**
     * @brief Returns the images attached to this framebuffer.
     */
    [[nodiscard]]
    const std::vector<Image*>& attachments() const
    {
        return m_attachments;
    }

private:
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
    std::vector<Image*> m_attachments;
    Device& m_device;
};

} // namespace LavaVK

#endif // LAVAVK_BUFFER_HPP