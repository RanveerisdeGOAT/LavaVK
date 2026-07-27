#ifndef LAVAVK_PIPLINE_HPP
#define LAVAVK_PIPLINE_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

namespace LavaVK {

    class Device;



    enum class ShaderType {
        Vertex,
        Fragment,
        Compute
    };

    class Shader {
    public:
        // Load from file (.spv or .vert / .frag / .comp GLSL source)
        Shader(Device& device, const std::string& filepath);

        // Construct directly from SPIR-V 32-bit word vector
        Shader(Device& device, const std::vector<uint32_t>& code);
        ~Shader();

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        [[nodiscard]] VkShaderModule native() const { return m_module; }

    private:
        void createShaderModule(const std::vector<uint32_t>& code);

        Device& m_device;
        VkShaderModule m_module{VK_NULL_HANDLE};
    };



    /**
     * @brief High-level abstraction for Vulkan descriptor types.
     */
    enum class DescriptorType : uint32_t
    {
        Sampler                   = 0, // VK_DESCRIPTOR_TYPE_SAMPLER
        CombinedImageSampler      = 1, // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
        SampledImage              = 2, // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
        StorageImage              = 3, // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
        UniformTexelBuffer        = 4, // VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
        StorageTexelBuffer        = 5, // VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
        UniformBuffer             = 6, // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        StorageBuffer             = 7, // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
        UniformBufferDynamic      = 8, // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
        StorageBufferDynamic      = 9, // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
        InputAttachment          = 10  // VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
    };

    /**
     * @brief Bitflags for shader stages where a descriptor or push constant is accessible.
     */
    enum ShaderStageFlags : uint32_t
    {
        STAGE_VERTEX_BIT                  = 0x00000001, // VK_SHADER_STAGE_VERTEX_BIT
        STAGE_TESSELLATION_CONTROL_BIT   = 0x00000002, // VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
        STAGE_TESSELLATION_EVALUATION_BIT= 0x00000004, // VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
        STAGE_GEOMETRY_BIT               = 0x00000008, // VK_SHADER_STAGE_GEOMETRY_BIT
        STAGE_FRAGMENT_BIT               = 0x00000010, // VK_SHADER_STAGE_FRAGMENT_BIT
        STAGE_COMPUTE_BIT                = 0x00000020, // VK_SHADER_STAGE_COMPUTE_BIT
        STAGE_ALL_GRAPHICS               = 0x0000001F, // VK_SHADER_STAGE_ALL_GRAPHICS
        STAGE_ALL                        = 0x7FFFFFFF  // VK_SHADER_STAGE_ALL
    };

    /**
     * @brief Clean descriptor set layout binding definition.
     */
    struct DescriptorSetLayoutBinding
    {
        uint32_t binding = 0;
        DescriptorType descriptorType = DescriptorType::UniformBuffer;
        uint32_t descriptorCount = 1;
        uint32_t stageFlags = ShaderStageFlags::STAGE_ALL_GRAPHICS;
    };

    /**
     * @brief Clean push constant range definition.
     */
    struct PushConstantRange
    {
        uint32_t stageFlags = ShaderStageFlags::STAGE_ALL_GRAPHICS;
        uint32_t offset = 0;
        uint32_t size = 0;
    };

    // Forward declaration
    class DescriptorWriter;



    class DescriptorSetLayout {
    public:
        class Builder {
        public:
            explicit Builder(Device& device) : m_device(device) {}

            Builder& addBinding(
                uint32_t binding,
                DescriptorType descriptorType,
                uint32_t stageFlags,
                uint32_t count = 1);

            std::unique_ptr<DescriptorSetLayout> build() const;

        private:
            Device& m_device;
            std::unordered_map<uint32_t, DescriptorSetLayoutBinding> m_bindings{};
        };

        DescriptorSetLayout(Device& device, std::unordered_map<uint32_t, DescriptorSetLayoutBinding> bindings);
        ~DescriptorSetLayout();

        DescriptorSetLayout(const DescriptorSetLayout&) = delete;
        DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

        [[nodiscard]] VkDescriptorSetLayout native() const { return m_layout; }
        [[nodiscard]] const std::unordered_map<uint32_t, DescriptorSetLayoutBinding>& getBindings() const { return m_bindings; }

    private:
        Device& m_device;
        VkDescriptorSetLayout m_layout{VK_NULL_HANDLE};
        std::unordered_map<uint32_t, DescriptorSetLayoutBinding> m_bindings;
    };



    class DescriptorPool {
    public:
        class Builder {
        public:
            explicit Builder(Device& device) : m_device(device) {}

            Builder& addPoolSize(DescriptorType descriptorType, uint32_t count);
            Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
            Builder& setMaxSets(uint32_t count);
            std::unique_ptr<DescriptorPool> build() const;

        private:
            Device& m_device;
            std::unordered_map<DescriptorType, uint32_t> m_poolCounts{};
            uint32_t m_maxSets = 1000;
            VkDescriptorPoolCreateFlags m_poolFlags = 0;
        };

        DescriptorPool(Device& device, uint32_t maxSets, VkDescriptorPoolCreateFlags flags, const std::vector<VkDescriptorPoolSize>& poolSizes);
        ~DescriptorPool();

        DescriptorPool(const DescriptorPool&) = delete;
        DescriptorPool& operator=(const DescriptorPool&) = delete;

        bool allocateDescriptorSet(VkDescriptorSetLayout layout, VkDescriptorSet& descriptorSet) const;
        void freeDescriptorSets(const std::vector<VkDescriptorSet>& descriptorSets) const;

        [[nodiscard]] VkDescriptorPool native() const { return m_pool; }

    private:
        friend class DescriptorWriter;

        Device& m_device;
        VkDescriptorPool m_pool{VK_NULL_HANDLE};
    };



    class DescriptorWriter {
    public:
        DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool);

        DescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
        DescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

        bool build(VkDescriptorSet& set);
        void overwrite(VkDescriptorSet& set);

    private:
        DescriptorSetLayout& m_setLayout;
        DescriptorPool& m_pool;
        std::vector<VkWriteDescriptorSet> m_writes;
    };



    class PipelineLayout {
    public:
        PipelineLayout(Device& device,
                       const std::vector<const DescriptorSetLayout*>& descriptorSetLayouts = {},
                       const std::vector<PushConstantRange>& pushConstantRanges = {});
        ~PipelineLayout();

        PipelineLayout(const PipelineLayout&) = delete;
        PipelineLayout& operator=(const PipelineLayout&) = delete;

        [[nodiscard]] VkPipelineLayout native() const { return m_layout; }

    private:
        Device& m_device;
        VkPipelineLayout m_layout{VK_NULL_HANDLE};
    };



    class RenderPass {
    public:
        RenderPass(Device& device, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
        RenderPass(Device& device, const VkRenderPassCreateInfo& createInfo);
        ~RenderPass();

        RenderPass(const RenderPass&) = delete;
        RenderPass& operator=(const RenderPass&) = delete;

        [[nodiscard]] VkRenderPass native() const { return m_renderPass; }

    private:
        Device& m_device;
        VkRenderPass m_renderPass{VK_NULL_HANDLE};
    };



    enum class PrimitiveTopology : uint32_t
    {
        POINTS                  = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        LINES                   = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
        LINE_STRIP              = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
        TRIANGLES               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        TRIANGLE_STRIP          = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        TRIANGLE_FAN            = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
        LINE_LIST_ADJACENCY     = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
        LINE_STRIP_ADJACENCY    = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,
        TRIANGLE_LIST_ADJACENCY = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
        TRIANGLE_STRIP_ADJACENCY= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,
        PATCHES                 = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST
    };

    enum class PolygonMode : uint32_t
    {
        FILL  = VK_POLYGON_MODE_FILL,
        LINE  = VK_POLYGON_MODE_LINE,
        POINT = VK_POLYGON_MODE_POINT
    };

    enum class CullMode : uint32_t
    {
        NONE           = VK_CULL_MODE_NONE,
        FRONT          = VK_CULL_MODE_FRONT_BIT,
        BACK           = VK_CULL_MODE_BACK_BIT,
        FRONT_AND_BACK = VK_CULL_MODE_FRONT_AND_BACK
    };

    enum class FrontFace : uint32_t
    {
        COUNTER_CLOCKWISE = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        CLOCKWISE         = VK_FRONT_FACE_CLOCKWISE
    };

    enum class CompareOperation : uint32_t
    {
        NEVER            = VK_COMPARE_OP_NEVER,
        LESS             = VK_COMPARE_OP_LESS,
        EQUAL            = VK_COMPARE_OP_EQUAL,
        LESS_OR_EQUAL    = VK_COMPARE_OP_LESS_OR_EQUAL,
        GREATER          = VK_COMPARE_OP_GREATER,
        NOT_EQUAL        = VK_COMPARE_OP_NOT_EQUAL,
        GREATER_OR_EQUAL = VK_COMPARE_OP_GREATER_OR_EQUAL,
        ALWAYS           = VK_COMPARE_OP_ALWAYS
    };

    enum class BlendFactor : uint32_t
    {
        ZERO                     = VK_BLEND_FACTOR_ZERO,
        ONE                      = VK_BLEND_FACTOR_ONE,
        SRC_COLOR                = VK_BLEND_FACTOR_SRC_COLOR,
        ONE_MINUS_SRC_COLOR      = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
        DST_COLOR                = VK_BLEND_FACTOR_DST_COLOR,
        ONE_MINUS_DST_COLOR      = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
        SRC_ALPHA                = VK_BLEND_FACTOR_SRC_ALPHA,
        ONE_MINUS_SRC_ALPHA      = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        DST_ALPHA                = VK_BLEND_FACTOR_DST_ALPHA,
        ONE_MINUS_DST_ALPHA      = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA
    };

    enum class BlendOperation : uint32_t
    {
        ADD              = VK_BLEND_OP_ADD,
        SUBTRACT         = VK_BLEND_OP_SUBTRACT,
        REVERSE_SUBTRACT = VK_BLEND_OP_REVERSE_SUBTRACT,
        MIN              = VK_BLEND_OP_MIN,
        MAX              = VK_BLEND_OP_MAX
    };

    struct GraphicsPipelineCreateInfo
    {
        Shader* vertexShader = nullptr;
        Shader* fragmentShader = nullptr;

        PipelineLayout* layout = nullptr;
        RenderPass* renderPass = nullptr;

        PrimitiveTopology topology = PrimitiveTopology::TRIANGLES;

        PolygonMode polygonMode = PolygonMode::FILL;
        CullMode cullMode = CullMode::BACK;
        FrontFace frontFace = FrontFace::COUNTER_CLOCKWISE;

        bool depthTest = true;
        bool depthWrite = true;
        CompareOperation depthCompare = CompareOperation::LESS;

        bool blending = false;
        BlendFactor srcColorBlendFactor = BlendFactor::SRC_ALPHA;
        BlendFactor dstColorBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
        BlendOperation colorBlendOperation = BlendOperation::ADD;

        BlendFactor srcAlphaBlendFactor = BlendFactor::ONE;
        BlendFactor dstAlphaBlendFactor = BlendFactor::ZERO;
        BlendOperation alphaBlendOperation = BlendOperation::ADD;

        bool dynamicViewport = true;
        bool dynamicScissor = true;

        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    };

    class GraphicsPipeline
    {
    public:

        GraphicsPipeline(
            Device& device,
            const GraphicsPipelineCreateInfo& info);

        ~GraphicsPipeline();

        GraphicsPipeline(const GraphicsPipeline&) = delete;
        GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

        GraphicsPipeline(GraphicsPipeline&& other) noexcept;
        GraphicsPipeline& operator=(GraphicsPipeline&& other) noexcept;

        void bind(VkCommandBuffer commandBuffer) const;

        [[nodiscard]]
        VkPipeline native() const
        {
            return m_pipeline;
        }

    private:

        Device& m_device;

        VkPipeline m_pipeline = VK_NULL_HANDLE;
    };

} // namespace LavaVK

#endif // LAVAVK_PIPLINE_HPP