#ifndef LAVAVK_DESCRIPTOR_H
#define LAVAVK_DESCRIPTOR_H
#include <unordered_map>
#include <vulkan/vulkan_core.h>

#include "Shader.hpp"
#include "Texture.hpp"
// #include "Texture.hpp"

namespace LavaVK {
    class Texture;
    class Buffer;
    class Image;
}

namespace LavaVK {
    /**
     * @brief High-level abstraction for Vulkan descriptor types.
     * Maps 1:1 to Vulkan `VkDescriptorType` values.
     */
    enum class DescriptorType : uint32_t {
        Sampler = 0, /**< VK_DESCRIPTOR_TYPE_SAMPLER */
        CombinedImageSampler = 1, /**< VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER */
        SampledImage = 2, /**< VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE */
        StorageImage = 3, /**< VK_DESCRIPTOR_TYPE_STORAGE_IMAGE */
        UniformTexelBuffer = 4, /**< VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER */
        StorageTexelBuffer = 5, /**< VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER */
        UniformBuffer = 6, /**< VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER */
        StorageBuffer = 7, /**< VK_DESCRIPTOR_TYPE_STORAGE_BUFFER */
        UniformBufferDynamic = 8, /**< VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC */
        StorageBufferDynamic = 9, /**< VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC */
        InputAttachment = 10 /**< VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT */
    };

    /**
     * @brief Describes a single binding within a descriptor set layout.
     */
    struct DescriptorSetLayoutBinding {
        uint32_t binding = 0; /**< Binding slot number in shader */
        DescriptorType descriptorType = DescriptorType::UniformBuffer; /**< Type of resource bound */
        uint32_t descriptorCount = 1; /**< Array count (1 for scalar) */
        uint32_t stageFlags = ShaderStageFlags::STAGE_ALL_GRAPHICS; /**< Accessible shader stages */
    };

    /**
     * @brief Defines a push constant memory range accessible in shaders.
     */
    struct PushConstantRange {
        uint32_t stageFlags = ShaderStageFlags::STAGE_ALL_GRAPHICS; /**< Target shader stages */
        uint32_t offset = 0; /**< Offset in bytes */
        uint32_t size = 0; /**< Size in bytes */
    };

    class DescriptorBuffer {
    public:
        DescriptorBuffer(
            Buffer &buffer,
            uint64_t offset,
            uint64_t range) {
            m_info.buffer = buffer.native();
            m_info.offset = offset;
            m_info.range = range;
        }

        [[nodiscard]]
        const VkDescriptorBufferInfo &native() const {
            return m_info;
        }

    private:
        VkDescriptorBufferInfo m_info{};
    };

    class DescriptorImage {
    public:
        DescriptorImage(
            Texture &tex,
            ImageLayout layout = ImageLayout::READ_ONLY_OPTIMAL) {
            m_info.sampler = tex.sampler();
            m_info.imageView = tex.image().view();
            m_info.imageLayout = static_cast<VkImageLayout>(layout);
        }

        [[nodiscard]]
        const VkDescriptorImageInfo &native() const {
            return m_info;
        }

    private:
        VkDescriptorImageInfo m_info{};
    };

    /**
     * @brief Abstraction around Vulkan's `VkDescriptorSetLayout`.
     * * Defines the interface between shader resource bindings and Vulkan descriptor sets.
     */
    class DescriptorSetLayout {
    public:
        /**
         * @brief Fluent builder helper for constructing `DescriptorSetLayout` instances.
         */
        class Builder {
        public:
            /**
             * @brief Constructs a new Builder for descriptor set layouts.
             * @param device LavaVK logical device reference.
             */
            explicit Builder(Device &device) : m_device(device) {
            }

            /**
             * @brief Adds a resource binding specification to the layout.
             * * @param binding Binding index defined in the shader layout (`layout(set = X, binding = Y)`).
             * @param descriptorType Type of descriptor resource (e.g., UniformBuffer, CombinedImageSampler).
             * @param stageFlags Bitmask of shader stages that can access this binding.
             * @param count Descriptor array size (defaults to 1).
             * @return Reference to this Builder instance for method chaining.
             */
            Builder &addBinding(
                uint32_t binding,
                DescriptorType descriptorType,
                uint32_t stageFlags,
                uint32_t count = 1);

            /**
             * @brief Builds and instantiates the `DescriptorSetLayout`.
             * @return `std::unique_ptr` owning the created `DescriptorSetLayout`.
             */
            std::unique_ptr<DescriptorSetLayout> build() const;

        private:
            Device &m_device;
            std::unordered_map<uint32_t, DescriptorSetLayoutBinding> m_bindings{};
        };

        /**
         * @brief Direct constructor for `DescriptorSetLayout`.
         * @param device Reference to the LavaVK logical device.
         * @param bindings Map of binding indices to layout binding definitions.
         */
        DescriptorSetLayout(Device &device, std::unordered_map<uint32_t, DescriptorSetLayoutBinding> bindings);

        /**
         * @brief Destroys the underlying `VkDescriptorSetLayout`.
         */
        ~DescriptorSetLayout();

        // Non-copyable
        DescriptorSetLayout(const DescriptorSetLayout &) = delete;

        DescriptorSetLayout &operator=(const DescriptorSetLayout &) = delete;

        /**
         * @brief Gets the native `VkDescriptorSetLayout` handle.
         * @return Native Vulkan layout handle.
         */
        [[nodiscard]] VkDescriptorSetLayout native() const { return m_layout; }

        /**
         * @brief Gets the configured bindings map.
         * @return Map of binding indices to DescriptorSetLayoutBinding structures.
         */
        [[nodiscard]] const std::unordered_map<uint32_t, DescriptorSetLayoutBinding> &getBindings() const {
            return m_bindings;
        }

    private:
        Device &m_device;
        VkDescriptorSetLayout m_layout{VK_NULL_HANDLE};
        std::unordered_map<uint32_t, DescriptorSetLayoutBinding> m_bindings;
    };

    // Type alias for native Vulkan DescriptorSet
    using DescriptorSet = VkDescriptorSet;


    static VkDescriptorType toVkDescriptorType(DescriptorType type) {
        switch (type) {
            case DescriptorType::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
            case DescriptorType::CombinedImageSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            case DescriptorType::SampledImage: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            case DescriptorType::StorageImage: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            case DescriptorType::UniformTexelBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            case DescriptorType::StorageTexelBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            case DescriptorType::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case DescriptorType::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case DescriptorType::UniformBufferDynamic: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            case DescriptorType::StorageBufferDynamic: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            case DescriptorType::InputAttachment: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        }
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }

    /**
     * @brief Manages a VkDescriptorPool and provides allocation/writing helper utilities.
     */
    class DescriptorPool {
    public:
        class Builder {
        public:
            explicit Builder(Device &device) : m_device(device) {
            }

            Builder &addPoolSize(DescriptorType descriptorType, uint32_t count);

            Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);

            Builder &setMaxSets(uint32_t count);

            std::unique_ptr<DescriptorPool> build() const;

        private:
            Device &m_device;
            std::unordered_map<DescriptorType, uint32_t> m_poolCounts{};
            uint32_t m_maxSets = 1000;
            VkDescriptorPoolCreateFlags m_poolFlags = 0;
        };

        /**
         * @brief Writer builder that allocates a descriptor set automatically, writes to it, and returns it.
         */
        class Writer {
        public:
            Writer(DescriptorPool &pool, DescriptorSetLayout &setLayout);

            Writer &writeBuffer(uint32_t binding, const DescriptorBuffer *bufferInfo);

            Writer &writeImage(uint32_t binding, const DescriptorImage *imageInfo);

            /**
             * @brief Allocates and writes the set, returning the created DescriptorSet handle.
             * @throw std::runtime_error if allocation fails.
             */
            DescriptorSet build();

            /**
             * @brief Overwrites an existing descriptor set instead of allocating a new one.
             */
            void overwrite(DescriptorSet set);

        private:
            DescriptorPool &m_pool;
            DescriptorSetLayout &m_setLayout;
            std::vector<VkWriteDescriptorSet> m_writes;
        };

        DescriptorPool(
            Device &device,
            uint32_t maxSets,
            VkDescriptorPoolCreateFlags flags,
            const std::vector<VkDescriptorPoolSize> &poolSizes
        );

        ~DescriptorPool();

        DescriptorPool(const DescriptorPool &) = delete;

        DescriptorPool &operator=(const DescriptorPool &) = delete;

        DescriptorSet allocateDescriptorSet(const DescriptorSetLayout &layout) const;

        void freeDescriptorSets(const std::vector<VkDescriptorSet> &descriptorSets) const;

        /**
         * @brief Creates a Writer helper bound to this pool and target layout.
         */
        Writer write(DescriptorSetLayout &setLayout) { return Writer(*this, setLayout); }

        [[nodiscard]] VkDescriptorPool native() const { return m_pool; }
        [[nodiscard]] Device &device() const { return m_device; }

    private:
        Device &m_device;
        VkDescriptorPool m_pool{VK_NULL_HANDLE};
    };

    /**
     * @brief Manages a single large descriptor set of textures indexed by integer
     * ID for "bindless" shader access.
     *
     * @details
     * Rather than binding one descriptor set per draw call, `BindlessTextureSet`
     * allocates one `VkDescriptorSet` with a single binding containing a large
     * array of `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER` descriptors
     * (#MAX_TEXTURES entries at binding #BINDING). Textures are registered via
     * #add(), which writes them into a free array slot and returns the slot's
     * index; that index can then be uploaded to the GPU (e.g. via push
     * constants or a per-instance/material buffer) and used in the shader to
     * index directly into the sampler array, avoiding descriptor set
     * (re)binding for every different texture.
     *
     * @note This relies on the physical device supporting descriptor indexing
     * (`GPUFeatures::descriptorIndexing`, in particular the `partiallyBound`
     * and `runtimeDescriptorArray` features) so that only some of the
     * #MAX_TEXTURES array slots need to be written at any given time; the
     * corresponding device features must be enabled when the #Device is
     * created. Descriptor writes performed by #add(), #remove(), and
     * #update() are not synchronized with in-flight GPU work: only call them
     * when the frame(s) that might reference the affected slot are known to
     * have finished executing, or when the device additionally supports
     * `updateAfterBind`. This class is neither copyable nor movable.
     *
     * Shader-side (GLSL), the corresponding binding looks like:
     * @code
     * layout(set = 0, binding = 0) uniform sampler2D bindlessTextures[];
     *
     * layout(push_constant) uniform PushConstants {
     *     uint textureId;
     * } pc;
     *
     * void main() {
     *     vec4 color = texture(bindlessTextures[nonuniformEXT(pc.textureId)], uv);
     * }
     * @endcode
     *
     * Example, registering textures and issuing a draw:
     * @code
     * LavaVK::BindlessTextureSet bindlessTextures(device);
     *
     * uint32_t brickId = bindlessTextures.add(brickTexture);
     * uint32_t stoneId = bindlessTextures.add(stoneTexture);
     *
     * cmd.bindDescriptorSets(pipelineLayout, LavaVK::PipelineBindPoint::Graphics,
     *                        { bindlessTextures.descriptorSet() });
     *
     * cmd.pushConstants(pipelineLayout, LavaVK::ShaderStageFlags::STAGE_FRAGMENT_BIT, brickId);
     * cmd.drawIndexed(indexCount);
     * @endcode
     */
    class BindlessTextureSet {
    public:
        /**
         * @brief Maximum number of textures that can be registered at once.
         * @details Determines the `descriptorCount` of the underlying array
         * binding; also the exclusive upper bound of the IDs returned by #add().
         */
        static constexpr uint32_t MAX_TEXTURES = 4096;

        /** @brief Descriptor set layout binding index of the bindless texture array. */
        static constexpr uint32_t BINDING = 0;

        /**
         * @brief Creates the descriptor set layout, backing pool, and descriptor
         * set used to hold up to #MAX_TEXTURES bindless textures.
         * @param device Logical device used to create the layout, pool, and set.
         * @throw std::runtime_error If layout, pool, or descriptor set creation fails.
         */
        explicit BindlessTextureSet(Device &device);

        /**
         * @brief Destroys the owned descriptor pool (and, with it, the descriptor
         * set allocated from it) and the descriptor set layout.
         */
        ~BindlessTextureSet() = default;

        /// @name Deleted Copy/Move Operations
        /// @{
        BindlessTextureSet(const BindlessTextureSet &) = delete;
        BindlessTextureSet &operator=(const BindlessTextureSet &) = delete;
        BindlessTextureSet(BindlessTextureSet &&) = delete;
        BindlessTextureSet &operator=(BindlessTextureSet &&) = delete;
        /// @}

        /**
         * @brief Registers a texture in the next free array slot.
         * @details Writes a `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER` descriptor
         * for @p texture into the returned slot at #BINDING. @p texture must
         * outlive its registration in this set (until #remove() is called or
         * this `BindlessTextureSet` is destroyed); no ownership is taken.
         * @param texture The texture to register. Must remain valid and unmoved
         * for as long as its slot is registered.
         * @return The array index (ID) @p texture was written to; pass this to
         * shaders to index the bindless array.
         * @throw std::runtime_error If #MAX_TEXTURES textures are already registered.
         */
        uint32_t add(Texture &texture);

        /**
         * @brief Frees a previously registered texture's slot for reuse.
         * @details Does not rewrite the underlying descriptor; the caller is
         * responsible for ensuring shaders no longer index @p id after removal
         * (e.g. by no longer issuing draws that reference it) since the
         * descriptor slot may point at a texture that has since been destroyed.
         * @param id A previously-returned ID from #add() that has not already been removed.
         * @throw std::out_of_range If @p id is out of range or not currently registered.
         */
        void remove(uint32_t id);

        /**
         * @brief Looks up the texture currently registered at a given slot.
         * @param id Array index (ID) to query.
         * @return Pointer to the registered #Texture, or `nullptr` if @p id is
         * out of range or has no texture currently registered.
         */
        Texture *get(uint32_t id) const;

        /**
         * @brief Rewrites the descriptor at a slot from its currently registered texture.
         * @details Useful when a #Texture's underlying image or sampler has been
         * recreated (e.g. after a resize) and the existing descriptor write at
         * @p id needs to be refreshed to point at the new resources.
         * @param id A previously-returned ID from #add() that has not since been removed.
         * @throw std::out_of_range If @p id is out of range or not currently registered.
         */
        void update(uint32_t id);

        /**
         * @brief Returns the underlying bindless descriptor set.
         * @return The `DescriptorSet` containing the bindless texture array at #BINDING.
         */
        [[nodiscard]] const DescriptorSet &descriptorSet() const { return m_set; }

        [[nodiscard]] const DescriptorSetLayout &layout() const { return m_layout; }

    private:
        /**
         * @brief Writes (or rewrites) the descriptor for a single array slot.
         * @param id Array index to write.
         * @param texture Texture whose image view/sampler are written into the slot.
         */
        void writeDescriptor(uint32_t id, Texture &texture) const;

        Device &m_device;

        DescriptorSetLayout m_layout;
        std::unique_ptr<DescriptorPool> m_pool;
        DescriptorSet m_set{};

        std::vector<Texture *> m_textures;
        std::vector<uint32_t> m_freeIndices;
    };
}

#endif //LAVAVK_DESCRIPTOR_H
