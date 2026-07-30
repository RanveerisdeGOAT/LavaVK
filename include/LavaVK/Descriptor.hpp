#ifndef LAVAVK_DESCRIPTOR_H
#define LAVAVK_DESCRIPTOR_H
#include <unordered_map>
#include <vulkan/vulkan_core.h>

#include "Shader.hpp"

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

    /**
     * @brief Manages allocation pool for Vulkan descriptor sets (`VkDescriptorPool`).
     */
    class DescriptorPool {
    public:
        /**
         * @brief Fluent builder helper for constructing `DescriptorPool` instances.
         */
        class Builder {
        public:
            /**
             * @brief Constructs a new Builder for descriptor pool creation.
             * @param device Reference to the LavaVK logical device.
             */
            explicit Builder(Device &device) : m_device(device) {
            }

            /**
             * @brief Configures maximum pool capacity for a given descriptor type.
             * @param descriptorType Type of descriptor to reserve slots for.
             * @param count Number of descriptors of this type to allocate space for.
             * @return Reference to this Builder instance.
             */
            Builder &addPoolSize(DescriptorType descriptorType, uint32_t count);

            /**
             * @brief Sets Vulkan creation flags (e.g., `VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT`).
             * @param flags Vulkan flags.
             * @return Reference to this Builder instance.
             */
            Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);

            /**
             * @brief Sets the maximum total number of descriptor sets allocatable from this pool.
             * @param count Maximum set count (defaults to 1000).
             * @return Reference to this Builder instance.
             */
            Builder &setMaxSets(uint32_t count);

            /**
             * @brief Builds and instantiates the `DescriptorPool`.
             * @return `std::unique_ptr` owning the created `DescriptorPool`.
             */
            std::unique_ptr<DescriptorPool> build() const;

        private:
            Device &m_device;
            std::unordered_map<DescriptorType, uint32_t> m_poolCounts{};
            uint32_t m_maxSets = 1000;
            VkDescriptorPoolCreateFlags m_poolFlags = 0;
        };

        /**
         * @brief Direct constructor for `DescriptorPool`.
         * @param device Reference to the LavaVK logical device.
         * @param maxSets Maximum descriptor sets allocatable.
         * @param flags Creation flags.
         * @param poolSizes Sizes for individual descriptor types.
         */
        DescriptorPool(Device &device, uint32_t maxSets, VkDescriptorPoolCreateFlags flags,
                       const std::vector<VkDescriptorPoolSize> &poolSizes);

        /**
         * @brief Destroys the underlying `VkDescriptorPool`.
         */
        ~DescriptorPool();

        // Non-copyable
        DescriptorPool(const DescriptorPool &) = delete;

        DescriptorPool &operator=(const DescriptorPool &) = delete;

        /**
         * @brief Allocates a descriptor set using a specified layout.
         * @param layout Layout configuration to allocate against.
         * @param descriptorSet [out] Output handle receiving the allocated descriptor set.
         * @return True if allocation succeeded, false otherwise.
         */
        bool allocateDescriptorSet(VkDescriptorSetLayout layout, VkDescriptorSet &descriptorSet) const;

        /**
         * @brief Frees one or more allocated descriptor sets back to this pool.
         * @param descriptorSets List of descriptor set handles to free.
         */
        void freeDescriptorSets(const std::vector<VkDescriptorSet> &descriptorSets) const;

        /**
         * @brief Gets the native Vulkan `VkDescriptorPool` handle.
         * @return Native VkDescriptorPool handle.
         */
        [[nodiscard]] VkDescriptorPool native() const { return m_pool; }

    private:
        friend class DescriptorWriter;

        Device &m_device;
        VkDescriptorPool m_pool{VK_NULL_HANDLE};
    };

    /**
     * @brief Helper class to bind buffers and image samplers to descriptor set slots.
     */
    class DescriptorWriter {
    public:
        /**
         * @brief Constructs a DescriptorWriter.
         * @param setLayout Target descriptor layout description.
         * @param pool Descriptor pool from which to allocate/update sets.
         */
        DescriptorWriter(DescriptorSetLayout &setLayout, DescriptorPool &pool);

        /**
         * @brief Enqueues a buffer binding write operation.
         * @param binding Target layout binding index.
         * @param bufferInfo Pointer to Vulkan descriptor buffer info.
         * @return Reference to this writer for chaining.
         */
        DescriptorWriter &writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo);

        /**
         * @brief Enqueues an image/sampler binding write operation.
         * @param binding Target layout binding index.
         * @param imageInfo Pointer to Vulkan descriptor image info.
         * @return Reference to this writer for chaining.
         */
        DescriptorWriter &writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo);

        /**
         * @brief Allocates a new descriptor set from the pool and writes the configured descriptor updates.
         * @param set [out] Output handle receiving the allocated and populated set.
         * @return True if allocation and writing succeeded.
         */
        bool build(VkDescriptorSet &set);

        /**
         * @brief Overwrites an existing allocated descriptor set with configured write operations.
         * @param set Target descriptor set handle to overwrite.
         */
        void overwrite(VkDescriptorSet &set);

    private:
        DescriptorSetLayout &m_setLayout;
        DescriptorPool &m_pool;
        std::vector<VkWriteDescriptorSet> m_writes;
    };

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
}

#endif //LAVAVK_DESCRIPTOR_H
