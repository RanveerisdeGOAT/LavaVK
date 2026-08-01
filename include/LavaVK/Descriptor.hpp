#ifndef LAVAVK_DESCRIPTOR_H
#define LAVAVK_DESCRIPTOR_H
#include <unordered_map>
#include <vulkan/vulkan_core.h>

#include "Shader.hpp"
#include "Texture.hpp"

namespace LavaVK {
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
}

#endif //LAVAVK_DESCRIPTOR_H
