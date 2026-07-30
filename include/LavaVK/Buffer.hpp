#ifndef LAVAVK_BUFFER_HPP
#define LAVAVK_BUFFER_HPP

#include <vector>
#include <vulkan/vulkan.h>
#include <cstring>
#include <stdexcept>
#include <utility>

#include "Core.hpp"
#include "Device.hpp"
#include "Instance.hpp"
#include "Error.hpp"

namespace LavaVK {
    class RenderPass;

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
        TRANSFER_DST = VK_IMAGE_USAGE_TRANSFER_DST_BIT, /**< Usable as the destination of a transfer (copy/blit) command. */
        SAMPLED = VK_IMAGE_USAGE_SAMPLED_BIT, /**< Usable as a sampled texture in a shader. */
        STORAGE = VK_IMAGE_USAGE_STORAGE_BIT, /**< Usable as a storage image for read/write shader access. */
        COLOR_ATTACHMENT = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, /**< Usable as a color attachment in a framebuffer. */
        DEPTH_ATTACHMENT = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, /**< Usable as a depth/stencil attachment in a framebuffer. */
        TRANSIENT = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, /**< Hints the implementation this attachment's contents need not persist. */
        INPUT_ATTACHMENT = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT /**< Usable as an input attachment, read by a shader within the same subpass. */
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
     * @brief Represents a Vulkan framebuffer.
     *
     * @details
     * A framebuffer is a collection of image attachments compatible with a
     * specific RenderPass. Attachments typically include a color image,
     * depth image, or additional render targets. `Framebuffer` creates the
     * underlying `VkFramebuffer` in its constructor from a list of #Image
     * attachments and a compatible #RenderPass, and destroys it in the
     * destructor. It does not own the `Image` attachments themselves —
     * those must outlive the `Framebuffer`.
     *
     * Example, building one framebuffer per swapchain image:
     * @code
     * std::vector<LavaVK::Image *> attachments = { &colorImage, &depthImage };
     * LavaVK::Framebuffer framebuffer(device, renderPass, attachments);
     * @endcode
     */
    class Framebuffer {
    public:
        /**
         * @brief Creates a framebuffer from a collection of image attachments.
         *
         * @details The framebuffer's width and height are taken from the
         * extent of the first entry in @p attachments; all attachments are
         * expected to share compatible dimensions with that first image.
         *
         * @param device Logical device used to create the framebuffer.
         * @param renderPass Compatible render pass.
         * @param attachments Images attached to the framebuffer.
         * @throw std::runtime_error If @p attachments is empty, contains an
         * invalid image, or if `vkCreateFramebuffer` fails.
         */
        Framebuffer(
            Device &device,
            RenderPass &renderPass,
            const std::vector<Image *> &attachments);

        /**
         * @brief Destroys the framebuffer.
         * @details Calls `vkDestroyFramebuffer` on the owned handle; does not
         * destroy the attached `Image` objects.
         */
        ~Framebuffer();

        // Framebuffer(const Framebuffer&) = delete;
        // Framebuffer& operator=(const Framebuffer&) = delete;

        /**
         * @brief Returns the native Vulkan framebuffer.
         * @return The underlying `VkFramebuffer` handle.
         */
        [[nodiscard]]
        VkFramebuffer native() const {
            return m_framebuffer;
        }

        /**
         * @brief Returns the images attached to this framebuffer.
         * @return Reference to the vector of attachment pointers passed at construction.
         */
        [[nodiscard]]
        const std::vector<Image *> &attachments() const {
            return m_attachments;
        }

    private:
        VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
        std::vector<Image *> m_attachments;
        Device &m_device;
    };

    /**
     * @brief Specifies how a #Buffer will be used by the GPU, mirroring `VkBufferUsageFlagBits`.
     * @details Multiple usage flags may be combined with the bitwise OR operator.
     */
    enum class BufferUsage : VkBufferUsageFlags {
        Vertex = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, /**< Usable as a vertex buffer source for draw calls. */
        Index = VK_BUFFER_USAGE_INDEX_BUFFER_BIT, /**< Usable as an index buffer source for indexed draw calls. */
        Uniform = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, /**< Usable as a uniform buffer bound to a shader. */
        Storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, /**< Usable as a storage buffer for read/write shader access. */

        TransferSrc = VK_BUFFER_USAGE_TRANSFER_SRC_BIT, /**< Usable as the source of a transfer (copy) command. */
        TransferDst = VK_BUFFER_USAGE_TRANSFER_DST_BIT, /**< Usable as the destination of a transfer (copy) command. */
    };

    /**
     * @brief Combines two BufferUsage flags.
     * @param a First set of usage flags.
     * @param b Second set of usage flags.
     * @return A #BufferUsage containing the bitwise OR of @p a and @p b.
     */
    inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
        return static_cast<BufferUsage>(
            static_cast<VkBufferUsageFlags>(a) |
            static_cast<VkBufferUsageFlags>(b));
    }

    /**
     * @brief High-level memory placement hint for a #Buffer, mapped internally to `VkMemoryPropertyFlags`.
     */
    enum class MemoryUsage {
        GPU, /**< Device-local memory only; fastest for the GPU but not directly writable from the CPU. */
        CPU, /**< Host-visible memory only; readable/writable from the CPU. */
        CPU_TO_GPU, /**< Host-visible memory intended for CPU writes the GPU will read (e.g. staging/upload buffers). */
        GPU_TO_CPU /**< Host-visible, cached memory intended for CPU reads of GPU-written data (e.g. readback buffers). */
    };

    /**
     * @brief Describes the properties of a buffer to be created.
     */
    struct BufferCreateInfo {
        /** @brief Size of the buffer, in bytes. */
        size_t size = 0;

        /** @brief Intended usage of the buffer (vertex, index, uniform, storage, transfer src/dst). */
        BufferUsage usage =
                BufferUsage::Vertex;

        /** @brief Memory placement hint controlling CPU/GPU visibility of the buffer's memory. */
        MemoryUsage memory =
                MemoryUsage::GPU;
    };

    /**
     * @brief Represents a Vulkan buffer (`VkBuffer`) and its backing device memory.
     *
     * @details
     * `Buffer` owns both the `VkBuffer` handle and the `VkDeviceMemory`
     * allocated to back it, created together in the constructor from a
     * #BufferCreateInfo and destroyed together in the destructor. For
     * memory placements that are host-visible (#MemoryUsage::CPU,
     * #MemoryUsage::CPU_TO_GPU, #MemoryUsage::GPU_TO_CPU), the buffer can be
     * written to directly via #map()/#unmap() or the #upload() convenience
     * overloads, which map, `memcpy`, and unmap in one call. `Buffer` is
     * move-only RAII; copying is disabled.
     *
     * Example:
     * @code
     * LavaVK::Buffer vertexBuffer(device, {
     *     .size = sizeof(Vertex) * vertices.size(),
     *     .usage = LavaVK::BufferUsage::Vertex,
     *     .memory = LavaVK::MemoryUsage::CPU_TO_GPU
     * });
     * vertexBuffer.upload(vertices);
     *
     * cmd.bindVertexBuffer(vertexBuffer);
     * @endcode
     */
    class Buffer {
    public:
        /**
         * @brief Constructs an empty buffer.
         * @details Leaves the buffer in an unowned state (`native()` returns
         * `VK_NULL_HANDLE`) until move-assigned from a constructed `Buffer`.
         */
        Buffer() = default;

        /**
         * @brief Creates a new Vulkan buffer and allocates its backing memory.
         * @param device Logical device used to create the buffer.
         * @param info Buffer creation parameters (size, usage, memory placement).
         * @throw std::runtime_error If buffer creation or memory allocation/binding fails.
         */
        Buffer(
            Device &device,
            const BufferCreateInfo &info);

        /**
         * @brief Destroys the buffer and frees its backing memory.
         * @details Unmaps the buffer first if currently mapped; a moved-from
         * `Buffer` is a no-op.
         */
        ~Buffer();

        Buffer(const Buffer &) = delete;

        Buffer &operator=(const Buffer &) = delete;

        /**
         * @brief Transfers ownership from another Buffer.
         * @param other The buffer being moved from (unnamed here); left in an empty, destructible state.
         */
        Buffer(Buffer &&) noexcept;

        /**
         * @brief Transfers ownership from another Buffer.
         * @param other The buffer being moved from (unnamed here); left in an empty, destructible state.
         * @return Reference to this buffer.
         */
        Buffer &operator=(Buffer &&) noexcept;

        /**
         * @brief Maps the buffer's memory into host address space.
         * @details Only valid for buffers created with a host-visible
         * #MemoryUsage (#MemoryUsage::CPU, #MemoryUsage::CPU_TO_GPU,
         * #MemoryUsage::GPU_TO_CPU). The returned pointer remains valid
         * until #unmap() is called.
         * @return Host-accessible pointer to the start of the buffer's memory.
         * @throw std::runtime_error If the buffer is not host-visible or `vkMapMemory` fails.
         */
        void *map();

        /**
         * @brief Unmaps previously mapped buffer memory.
         * @details No-op if the buffer is not currently mapped.
         */
        void unmap();

        /**
         * @brief Uploads raw bytes into the buffer.
         * @details Maps the buffer, copies @p bytes bytes from @p data via
         * `memcpy`, then unmaps it again. Intended for host-visible buffers;
         * see #map() for applicability.
         * @param data Pointer to the source data to copy from.
         * @param bytes Number of bytes to copy, starting at the beginning of the buffer.
         * @throw std::runtime_error If the buffer is not host-visible or mapping fails.
         */
        void upload(const void *data, size_t bytes);

        /**
         * @brief Uploads a single object's bytes into the buffer.
         * @tparam T Trivially-copyable type of the object being uploaded.
         * @param object The object to copy into the buffer.
         */
        template<typename T>
        void upload(const T &object) {
            upload(&object, sizeof(T));
        }

        /**
         * @brief Uploads the contents of a `std::vector` into the buffer.
         * @tparam T Trivially-copyable element type of the vector.
         * @param data The vector whose contents are copied into the buffer.
         */
        template<typename T>
        void upload(const std::vector<T> &data) {
            upload(data.data(), data.size() * sizeof(T));
        }

        /**
         * @brief Returns the native Vulkan buffer handle.
         * @return The underlying `VkBuffer` handle, or `VK_NULL_HANDLE` if unconstructed/moved-from.
         */
        VkBuffer native() const { return m_buffer; }

        /**
         * @brief Returns the size of the buffer.
         * @return The buffer's size in bytes, as given at construction.
         */
        size_t size() const { return m_size; }

        /**
         * @brief Returns the usage flags this buffer was created with.
         * @return The #BufferUsage this buffer was created with.
         */
        BufferUsage buffer() const { return m_usage; }

        /**
         * @brief Returns the memory placement this buffer was created with.
         * @return The #MemoryUsage this buffer was created with.
         */
        MemoryUsage memory() const { return m_memoryUsage; }

    private:
        Device *m_device = nullptr;

        VkBuffer m_buffer = VK_NULL_HANDLE;

        VkDeviceMemory m_memory = VK_NULL_HANDLE;

        void *m_mapped = nullptr;

        size_t m_size = 0;

        BufferUsage m_usage;

        MemoryUsage m_memoryUsage;

        /**
         * @brief Finds a physical device memory type index matching the requested properties.
         * @param physical Physical device to query available memory types from.
         * @param uint32 Bitmask of memory type indices acceptable for the resource
         * (from `VkMemoryRequirements::memoryTypeBits`).
         * @param properties Required `VkMemoryPropertyFlags` the memory type must support.
         * @return Index of a suitable memory type.
         * @throw std::runtime_error If no matching memory type is found.
         */
        static uint32_t findMemoryType(VkPhysicalDevice physical, uint32_t uint32, VkFlags properties);
    };

    /**
     * @brief Primary template declaration for extracting member-pointer metadata; see the
     * pointer-to-member specialization below for the actual implementation.
     * @tparam MemberPtr A pointer-to-member value (e.g. `&Vertex::position`).
     */
    template<auto MemberPtr>
    struct MemberTraits;

    /**
     * @brief Compile-time helper that extracts the byte offset of a struct member from its pointer-to-member.
     * @details Specializes #MemberTraits for the case where `MemberPtr` is a
     * pointer to a member `M` of struct `S`. Used by #VertexLayout::attribute()
     * to compute a vertex attribute's byte offset directly from a
     * `&Vertex::field` expression, without the caller needing `offsetof`.
     * @tparam M Type of the pointed-to member.
     * @tparam S Type of the struct containing the member.
     * @tparam MemberPtr The pointer-to-member itself.
     */
    template<typename M, typename S, M S::* MemberPtr>
    struct MemberTraits<MemberPtr> {
        /** @brief The type of the member pointed to by `MemberPtr`. */
        using MemberType = M;
        /** @brief The struct type containing the member. */
        using StructType = S;

        /**
         * @brief Calculates byte offset of the member inside the struct.
         * @details Constructs a scratch, uninitialized instance of `S` on the
         * stack and measures the address difference between the member and
         * the start of the struct.
         * @return Offset, in bytes, of the member referenced by `MemberPtr` within `S`.
         */
        static size_t offset() {
            alignas(S) char buffer[sizeof(S)];
            S *dummy = reinterpret_cast<S *>(buffer);
            return static_cast<size_t>(
                reinterpret_cast<const char *>(&(dummy->*MemberPtr)) - buffer
            );
        }
    };

    /**
     * @brief Builder describing a vertex buffer's binding and per-attribute layout for a pipeline.
     *
     * @details
     * `VertexLayout` produces the `VkVertexInputBindingDescription` and
     * `VkVertexInputAttributeDescription` list a `GraphicsPipeline` needs to
     * interpret vertex buffer data, without the caller manually computing
     * strides or `offsetof`-style byte offsets. #create() establishes the
     * binding (stride taken from `sizeof(Vertex)`), and chained calls to
     * #attribute() add one attribute per struct member, using a
     * pointer-to-member template argument so the byte offset is computed
     * automatically via #MemberTraits.
     *
     * Example:
     * @code
     * struct Vertex {
     *     glm::vec3 position;
     *     glm::vec3 color;
     * };
     *
     * LavaVK::VertexLayout vertexLayout = LavaVK::VertexLayout::create<Vertex>()
     *     .attribute<&Vertex::position>(0, LavaVK::Format(LavaVK::ChannelOrder::RGB, LavaVK::BitDepth::B32, LavaVK::NumericType::Float))
     *     .attribute<&Vertex::color>(1, LavaVK::Format(LavaVK::ChannelOrder::RGB, LavaVK::BitDepth::B32, LavaVK::NumericType::Float));
     * @endcode
     */
    class VertexLayout {
    public:
        /**
         * @brief Creates a vertex layout bound to a given binding slot, with stride derived from `Vertex`.
         * @tparam Vertex The vertex struct type this layout describes; its
         * size (via `sizeof(Vertex)`) becomes the binding's stride.
         * @param binding Vertex input binding slot this layout describes.
         * @param inputRate Whether vertex data advances per-vertex or per-instance.
         * @return A new #VertexLayout with its binding description populated;
         * call #attribute() on the result to add per-member attributes.
         */
        template<typename Vertex>
        static VertexLayout create(uint32_t binding = 0, VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX) {
            VertexLayout layout;
            layout.m_stride = sizeof(Vertex);
            layout.m_bindingIndex = binding;

            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = binding;
            bindingDescription.stride = static_cast<uint32_t>(sizeof(Vertex));
            bindingDescription.inputRate = inputRate;

            layout.m_bindings.push_back(bindingDescription);

            return layout;
        }

        /**
         * @brief Adds a vertex attribute derived from a struct member pointer.
         * @details The attribute's byte offset within the vertex struct is
         * computed automatically via #MemberTraits from `MemberPtr`, so
         * callers do not need to specify it manually.
         * @tparam MemberPtr Pointer-to-member of the vertex struct this
         * attribute reads from (e.g. `&Vertex::position`).
         * @param location Shader input location this attribute binds to.
         * @param format Vulkan-compatible #Format describing the attribute's data layout.
         * @return Reference to this `VertexLayout`, to allow chained `.attribute<...>(...)` calls.
         */
        template<auto MemberPtr>
        VertexLayout &attribute(uint32_t location, Format format) {
            using Traits = MemberTraits<MemberPtr>;

            VkVertexInputAttributeDescription attr{};
            attr.location = location;
            attr.binding = m_bindingIndex; // Uses the configured binding slot!
            attr.format = static_cast<VkFormat>(format);
            attr.offset = static_cast<uint32_t>(Traits::offset());

            m_attributes.push_back(attr);

            return *this;
        }

        /**
         * @brief Returns the vertex input binding slot this layout describes.
         * @return The binding index passed to #create().
         */
        [[nodiscard]]
        uint32_t bindingIndex() const {
            return m_bindingIndex;
        }

        /**
         * @brief Returns the byte stride between consecutive vertices.
         * @return The stride, in bytes, derived from `sizeof(Vertex)` in #create().
         */
        [[nodiscard]]
        uint32_t stride() const {
            return m_stride;
        }

        /**
         * @brief Returns the binding description(s) for this layout.
         * @return Reference to the vector of `VkVertexInputBindingDescription`
         * entries (populated with a single entry by #create()).
         */
        [[nodiscard]]
        const std::vector<VkVertexInputBindingDescription> &bindings() const {
            return m_bindings;
        }

        /**
         * @brief Returns the attribute descriptions added to this layout.
         * @return Reference to the vector of `VkVertexInputAttributeDescription`
         * entries added via #attribute().
         */
        [[nodiscard]]
        const std::vector<VkVertexInputAttributeDescription> &attributes() const {
            return m_attributes;
        }

    private:
        uint32_t m_stride = 0;
        uint32_t m_bindingIndex = 0;

        std::vector<VkVertexInputBindingDescription> m_bindings;
        std::vector<VkVertexInputAttributeDescription> m_attributes;
    };
} // namespace LavaVK

#endif // LAVAVK_BUFFER_HPP