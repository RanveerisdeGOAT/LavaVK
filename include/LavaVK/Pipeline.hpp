#ifndef LAVAVK_PIPELINE_HPP
#define LAVAVK_PIPELINE_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

#include "Instance.hpp"

namespace LavaVK {
    class Format;
}

namespace LavaVK {
    class Device;

    /**
     * @brief Shader types supported by the LavaVK abstraction layer.
     */
    enum class ShaderType {
        Vertex, /**< Vertex shader (.vert or SPIR-V) */
        Fragment, /**< Fragment/Pixel shader (.frag or SPIR-V) */
        Compute /**< Compute shader (.comp or SPIR-V) */
    };

    /**
     * @brief Encapsulates a Vulkan shader module (`VkShaderModule`).
     * * Supports loading directly from raw SPIR-V binaries (`.spv`), raw SPIR-V
     * byte vectors, or dynamically compiling GLSL source files (`.vert`, `.frag`, `.comp`)
     * using embedded Shaderc compiler support.
     */
    class Shader {
    public:
        /**
         * @brief Loads and constructs a shader module from a file path.
         * * Automatically detects file extension (`.spv` for compiled SPIR-V,
         * `.vert`/`.frag`/`.comp` for GLSL source).
         * * @param device Reference to the logical LavaVK device.
         * @param filepath Path to the shader file on disk.
         * @throws std::runtime_error If file opening or compilation fails.
         */
        Shader(Device &device, const std::string &filepath);

        /**
         * @brief Constructs a shader module directly from a vector of SPIR-V 32-bit words.
         * * @param device Reference to the logical LavaVK device.
         * @param code SPIR-V bytecode binary vector.
         * @throws std::runtime_error If Vulkan shader module creation fails.
         */
        Shader(Device &device, const std::vector<uint32_t> &code);

        /**
         * @brief Destroys the underlying `VkShaderModule`.
         */
        ~Shader();

        // Non-copyable
        Shader(const Shader &) = delete;

        Shader &operator=(const Shader &) = delete;

        /**
         * @brief Retrieves the native Vulkan `VkShaderModule` handle.
         * @return Raw VkShaderModule handle.
         */
        [[nodiscard]] VkShaderModule native() const { return m_module; }

    private:
        /**
         * @brief Internal helper to allocate the native Vulkan shader module.
         * @param code SPIR-V bytecode vector.
         */
        void createShaderModule(const std::vector<uint32_t> &code);

        Device &m_device;
        VkShaderModule m_module{VK_NULL_HANDLE};
    };

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
     * @brief Bitflags specifying shader stage accessibility for descriptors and push constants.
     */
    enum ShaderStageFlags : uint32_t {
        STAGE_VERTEX_BIT = 0x00000001, /**< VK_SHADER_STAGE_VERTEX_BIT */
        STAGE_TESSELLATION_CONTROL_BIT = 0x00000002, /**< VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT */
        STAGE_TESSELLATION_EVALUATION_BIT = 0x00000004, /**< VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT */
        STAGE_GEOMETRY_BIT = 0x00000008, /**< VK_SHADER_STAGE_GEOMETRY_BIT */
        STAGE_FRAGMENT_BIT = 0x00000010, /**< VK_SHADER_STAGE_FRAGMENT_BIT */
        STAGE_COMPUTE_BIT = 0x00000020, /**< VK_SHADER_STAGE_COMPUTE_BIT */
        STAGE_ALL_GRAPHICS = 0x0000001F, /**< All graphics stages combined */
        STAGE_ALL = 0x7FFFFFFF /**< All pipeline stages */
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

    // Forward declaration
    class DescriptorWriter;

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

    /**
     * @brief Encapsulates a Vulkan `VkPipelineLayout`.
     * * Defines descriptor set layouts and push constants used across pipeline stages.
     */
    class PipelineLayout {
    public:
        /**
         * @brief Constructs a pipeline layout.
         * * @param device Reference to the LavaVK logical device.
         * @param descriptorSetLayouts Collection of descriptor set layouts bound to this pipeline layout.
         * @param pushConstantRanges Push constant ranges accessible by this pipeline layout.
         */
        PipelineLayout(Device &device,
                       const std::vector<const DescriptorSetLayout *> &descriptorSetLayouts = {},
                       const std::vector<PushConstantRange> &pushConstantRanges = {});

        /**
         * @brief Destroys the underlying `VkPipelineLayout`.
         */
        ~PipelineLayout();

        // Non-copyable
        PipelineLayout(const PipelineLayout &) = delete;

        PipelineLayout &operator=(const PipelineLayout &) = delete;

        /**
         * @brief Gets the native Vulkan `VkPipelineLayout` handle.
         * @return Native VkPipelineLayout handle.
         */
        [[nodiscard]] VkPipelineLayout native() const { return m_layout; }

    private:
        Device &m_device;
        VkPipelineLayout m_layout{VK_NULL_HANDLE};
    };

    /**
     * @brief Wrapper for Vulkan `VkRenderPass`.
     * * Configures attachments (color, depth/stencil), subpasses, and dependencies.
     */
    class RenderPass {
    public:
        /**
         * @brief Helper constructor for a standard single-subpass color + depth render pass.
         * * @param device Reference to the LavaVK logical device.
         * @param colorFormat Format for the color attachment.
         * @param depthFormat Format for depth attachment (`VK_FORMAT_UNDEFINED` to disable depth).
         * @param samples Multisample count flag (defaults to 1 sample).
         */
        RenderPass(Device &device, Format colorFormat, Format depthFormat = Format(ChannelOrder::Undefined, BitDepth::Undefined, NumericType::Undefined),
                   VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);

        /**
         * @brief Explicit constructor using full Vulkan create info.
         * * @param device Reference to the LavaVK logical device.
         * @param createInfo Raw Vulkan `VkRenderPassCreateInfo` structure.
         */
        RenderPass(Device &device, const VkRenderPassCreateInfo &createInfo);

        /**
         * @brief Destroys the underlying `VkRenderPass`.
         */
        ~RenderPass();

        // Non-copyable
        RenderPass(const RenderPass &) = delete;

        RenderPass &operator=(const RenderPass &) = delete;

        /**
         * @brief Get color format.
         */
        VkFormat getColorFormat() const { return m_colorFormat; }

        /**
         * @brief Get depth format.
         */
        VkFormat getDepthFormat() const { return m_depthFormat; }

        /**
         * @brief Gets the native Vulkan `VkRenderPass` handle.
         * @return Native VkRenderPass handle.
         */
        [[nodiscard]] VkRenderPass native() const { return m_renderPass; }

    private:
        Device &m_device;
        VkRenderPass m_renderPass{VK_NULL_HANDLE};
        VkFormat m_depthFormat{VK_FORMAT_UNDEFINED};
        VkFormat m_colorFormat{VK_FORMAT_UNDEFINED};
    };

    /** @brief Primitive assembly topology type. */
    enum class Topology : uint32_t {
        POINTS = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        LINES = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
        LINE_STRIP = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
        TRIANGLES = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        TRIANGLE_STRIP = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        TRIANGLE_FAN = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
        LINE_LIST_ADJACENCY = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
        LINE_STRIP_ADJACENCY = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,
        TRIANGLE_LIST_ADJACENCY = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
        TRIANGLE_STRIP_ADJACENCY = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,
        PATCHES = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST
    };

    /** @brief Rasterization polygon rendering mode. */
    enum class PolygonMode : uint32_t {
        FILL = VK_POLYGON_MODE_FILL,
        LINE = VK_POLYGON_MODE_LINE,
        POINT = VK_POLYGON_MODE_POINT
    };

    /** @brief Face culling options. */
    enum class CullMode : uint32_t {
        NONE = VK_CULL_MODE_NONE,
        FRONT = VK_CULL_MODE_FRONT_BIT,
        BACK = VK_CULL_MODE_BACK_BIT,
        FRONT_AND_BACK = VK_CULL_MODE_FRONT_AND_BACK
    };

    /** @brief Vertex winding order determining front-facing polygons. */
    enum class FrontFace : uint32_t {
        COUNTER_CLOCKWISE = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        CLOCKWISE = VK_FRONT_FACE_CLOCKWISE
    };

    /** @brief Depth/Stencil comparison operations. */
    enum class CompareOperation : uint32_t {
        NEVER = VK_COMPARE_OP_NEVER,
        LESS = VK_COMPARE_OP_LESS,
        EQUAL = VK_COMPARE_OP_EQUAL,
        LESS_OR_EQUAL = VK_COMPARE_OP_LESS_OR_EQUAL,
        GREATER = VK_COMPARE_OP_GREATER,
        NOT_EQUAL = VK_COMPARE_OP_NOT_EQUAL,
        GREATER_OR_EQUAL = VK_COMPARE_OP_GREATER_OR_EQUAL,
        ALWAYS = VK_COMPARE_OP_ALWAYS
    };

    /** @brief Color blending factors for source and destination channels. */
    enum class BlendFactor : uint32_t {
        ZERO = VK_BLEND_FACTOR_ZERO,
        ONE = VK_BLEND_FACTOR_ONE,
        SRC_COLOR = VK_BLEND_FACTOR_SRC_COLOR,
        ONE_MINUS_SRC_COLOR = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
        DST_COLOR = VK_BLEND_FACTOR_DST_COLOR,
        ONE_MINUS_DST_COLOR = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
        SRC_ALPHA = VK_BLEND_FACTOR_SRC_ALPHA,
        ONE_MINUS_SRC_ALPHA = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        DST_ALPHA = VK_BLEND_FACTOR_DST_ALPHA,
        ONE_MINUS_DST_ALPHA = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA
    };

    /** @brief Arithmetic operations for color blending. */
    enum class BlendOperation : uint32_t {
        ADD = VK_BLEND_OP_ADD,
        SUBTRACT = VK_BLEND_OP_SUBTRACT,
        REVERSE_SUBTRACT = VK_BLEND_OP_REVERSE_SUBTRACT,
        MIN = VK_BLEND_OP_MIN,
        MAX = VK_BLEND_OP_MAX
    };

    /**
     * @brief Comprehensive configuration structure for creating a `GraphicsPipeline`.
     */
    struct GraphicsPipelineCreateInfo {
        Shader *vertexShader = nullptr; /**< Required vertex shader stage */
        Shader *fragmentShader = nullptr; /**< Required fragment shader stage */

        PipelineLayout *layout = nullptr; /**< Required pipeline layout configuration */
        RenderPass *renderPass = nullptr; /**< Compatible render pass */

        Topology topology = Topology::TRIANGLES; /**< Primitive topology type */

        PolygonMode polygonMode = PolygonMode::FILL; /**< Rasterization mode */
        CullMode cullMode = CullMode::BACK; /**< Face culling mode */
        FrontFace frontFace = FrontFace::COUNTER_CLOCKWISE; /**< Front-facing polygon winding order */

        bool depthTest = true; /**< Enable depth testing */
        bool depthWrite = true; /**< Enable depth buffer updates */
        CompareOperation depthCompare = CompareOperation::LESS; /**< Depth comparison condition */

        bool blending = false; /**< Enable color blending */
        BlendFactor srcColorBlendFactor = BlendFactor::SRC_ALPHA; /**< Source color factor */
        BlendFactor dstColorBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA; /**< Destination color factor */
        BlendOperation colorBlendOperation = BlendOperation::ADD; /**< Color blend operation */

        BlendFactor srcAlphaBlendFactor = BlendFactor::ONE; /**< Source alpha factor */
        BlendFactor dstAlphaBlendFactor = BlendFactor::ZERO; /**< Destination alpha factor */
        BlendOperation alphaBlendOperation = BlendOperation::ADD; /**< Alpha blend operation */

        bool dynamicViewport = true; /**< Enable dynamic viewport state (`vkCmdSetViewport`) */
        bool dynamicScissor = true; /**< Enable dynamic scissor state (`vkCmdSetScissor`) */

        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT; /**< Multisample sample count */
    };

    /**
     * @brief Abstraction around Vulkan's graphics pipeline (`VkPipeline`).
     * * Manages full graphics state setup including shaders, rasterization,
     * multisampling, depth/stencil, blending, and dynamic state bindings.
     */
    class GraphicsPipeline {
    public:
        /**
         * @brief Constructs and compiles a native Vulkan graphics pipeline.
         * * @param device Reference to the LavaVK logical device.
         * @param info Pipeline configuration descriptor.
         * @throws std::runtime_error If required parameters are missing or Vulkan pipeline compilation fails.
         */
        GraphicsPipeline(
            Device &device,
            const GraphicsPipelineCreateInfo &info);

        /**
         * @brief Destroys the underlying `VkPipeline`.
         */
        ~GraphicsPipeline();

        // Non-copyable
        GraphicsPipeline(const GraphicsPipeline &) = delete;

        GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;

        // Moveable
        GraphicsPipeline(GraphicsPipeline &&other) noexcept;

        GraphicsPipeline &operator=(GraphicsPipeline &&other) noexcept;

        /**
         * @brief Binds this graphics pipeline to a command buffer (`vkCmdBindPipeline`).
         * @param commandBuffer Active Vulkan command buffer handle.
         */
        void bind(VkCommandBuffer commandBuffer) const;

        /**
         * @brief Gets the native Vulkan `VkPipeline` handle.
         * @return Native VkPipeline handle.
         */
        [[nodiscard]]
        VkPipeline native() const {
            return m_pipeline;
        }

    private:
        Device &m_device;
        VkPipeline m_pipeline = VK_NULL_HANDLE;
    };

    enum class PipelineStage : VkPipelineStageFlags {
        None                   = 0,
        TopOfPipe              = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        DrawIndirect           = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
        VertexInput            = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        VertexShader           = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        TessellationControl    = VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT,
        TessellationEvaluation = VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT,
        GeometryShader         = VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,
        FragmentShader         = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        EarlyFragmentTests     = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        LateFragmentTests      = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        ColorAttachmentOutput  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        ComputeShader          = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        Transfer               = VK_PIPELINE_STAGE_TRANSFER_BIT,
        BottomOfPipe           = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        Host                   = VK_PIPELINE_STAGE_HOST_BIT,
        AllGraphics            = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
        AllCommands            = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
    };

    inline PipelineStage operator|(PipelineStage lhs, PipelineStage rhs) {
        return static_cast<PipelineStage>(
            static_cast<VkPipelineStageFlags>(lhs) | static_cast<VkPipelineStageFlags>(rhs)
        );
    }

} // namespace LavaVK

#endif // LAVAVK_PIPELINE_HPP
