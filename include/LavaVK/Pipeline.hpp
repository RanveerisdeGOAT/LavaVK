#ifndef LAVAVK_PIPELINE_HPP
#define LAVAVK_PIPELINE_HPP

#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <array>
#include <filesystem>
#include <shaderc/shaderc.hpp>

#include "Core.hpp"
#include "Device.hpp"
#include "Error.hpp"
#include "Shader.hpp"
#include "Descriptor.hpp"

namespace LavaVK {
    class VertexLayout;
    class Format;

    /**
     * @brief Encapsulates a Vulkan `VkPipelineLayout`.
     *
     * @details
     * Defines descriptor set layouts and push constants used across pipeline
     * stages. A `PipelineLayout` is required to construct both a
     * `GraphicsPipeline` and to record `pushConstants`/descriptor-binding
     * commands against a command buffer, since it declares which resources
     * and push constant ranges shaders may access. `PipelineLayout` is
     * non-copyable RAII: the underlying `VkPipelineLayout` is created in the
     * constructor and destroyed in the destructor.
     *
     * Example:
     * @code
     * LavaVK::PushConstantRange mvpRange{
     *     .stageFlags = LavaVK::STAGE_VERTEX_BIT,
     *     .offset = 0,
     *     .size = sizeof(glm::mat4)
     * };
     * LavaVK::PipelineLayout pipelineLayout(device, {}, { mvpRange });
     * @endcode
     */
    class PipelineLayout {
    public:
        /**
         * @brief Constructs a pipeline layout.
         * @param device Reference to the LavaVK logical device.
         * @param descriptorSetLayouts Collection of descriptor set layouts bound to this pipeline layout.
         * @param pushConstantRanges Push constant ranges accessible by this pipeline layout.
         * @throw std::runtime_error If `vkCreatePipelineLayout` fails.
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
     *
     * @details
     * Configures attachments (color, depth/stencil), subpasses, and
     * dependencies. Most applications only need a single color (+ optional
     * depth) attachment render pass, provided by the convenience
     * constructor; applications with more complex attachment layouts
     * (multiple subpasses, input attachments, MSAA resolve targets, ...)
     * can instead supply a raw `VkRenderPassCreateInfo` directly.
     * `RenderPass` is non-copyable RAII: the underlying `VkRenderPass` is
     * created in the constructor and destroyed in the destructor.
     *
     * Example:
     * @code
     * LavaVK::RenderPass renderPass(
     *     device,
     *     LavaVK::Format(LavaVK::ChannelOrder::BGRA, LavaVK::BitDepth::B8, LavaVK::NumericType::Srgb),
     *     LavaVK::Format(LavaVK::ChannelOrder::D, LavaVK::BitDepth::B32, LavaVK::NumericType::Float)
     * );
     * @endcode
     */
    class RenderPass {
    public:
        /**
         * @brief Helper constructor for a standard single-subpass color + depth render pass.
         * @param device Reference to the LavaVK logical device.
         * @param colorFormat Format for the color attachment.
         * @param depthFormat Format for depth attachment (`VK_FORMAT_UNDEFINED` to disable depth).
         * @param samples Multisample count flag (defaults to 1 sample).
         * @throw std::runtime_error If `vkCreateRenderPass` fails.
         */
        RenderPass(Device &device, Format colorFormat,
                   Format depthFormat = Format(ChannelOrder::Undefined, BitDepth::Undefined, NumericType::Undefined),
                   VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);

        /**
         * @brief Explicit constructor using full Vulkan create info.
         * @details Intended for render passes whose attachment/subpass
         * layout does not fit the single-color-plus-depth convenience
         * constructor above (e.g. multiple subpasses or additional
         * attachments); the caller is responsible for filling in
         * @p createInfo entirely.
         * @param device Reference to the LavaVK logical device.
         * @param createInfo Raw Vulkan `VkRenderPassCreateInfo` structure.
         * @throw std::runtime_error If `vkCreateRenderPass` fails.
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
         * @return The `VkFormat` used for this render pass's color attachment.
         */
        VkFormat getColorFormat() const { return m_colorFormat; }

        /**
         * @brief Get depth format.
         * @return The `VkFormat` used for this render pass's depth attachment,
         * or `VK_FORMAT_UNDEFINED` if depth is disabled.
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

    /** @brief Primitive assembly topology type, mirroring `VkPrimitiveTopology`. */
    enum class Topology : uint32_t {
        POINTS = VK_PRIMITIVE_TOPOLOGY_POINT_LIST, /**< Each vertex is a separate point. */
        LINES = VK_PRIMITIVE_TOPOLOGY_LINE_LIST, /**< Each pair of vertices forms an independent line. */
        LINE_STRIP = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, /**< Consecutive vertices form a connected line strip. */
        TRIANGLES = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, /**< Each triplet of vertices forms an independent triangle. */
        TRIANGLE_STRIP = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, /**< Consecutive vertices form a connected triangle strip. */
        TRIANGLE_FAN = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, /**< Vertices form a triangle fan around the first vertex. */
        LINE_LIST_ADJACENCY = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY, /**< Line list with adjacency information, for geometry shaders. */
        LINE_STRIP_ADJACENCY = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY, /**< Line strip with adjacency information, for geometry shaders. */
        TRIANGLE_LIST_ADJACENCY = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY, /**< Triangle list with adjacency information, for geometry shaders. */
        TRIANGLE_STRIP_ADJACENCY = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY, /**< Triangle strip with adjacency information, for geometry shaders. */
        PATCHES = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST /**< Vertices form patches, for tessellation. */
    };

    /** @brief Rasterization polygon rendering mode, mirroring `VkPolygonMode`. */
    enum class PolygonMode : uint32_t {
        FILL = VK_POLYGON_MODE_FILL, /**< Polygons are rasterized as filled regions. */
        LINE = VK_POLYGON_MODE_LINE, /**< Polygon edges are rasterized as wireframe lines. */
        POINT = VK_POLYGON_MODE_POINT /**< Polygon vertices are rasterized as points. */
    };

    /** @brief Face culling options, mirroring `VkCullModeFlagBits`. */
    enum class CullMode : uint32_t {
        NONE = VK_CULL_MODE_NONE, /**< No faces are culled. */
        FRONT = VK_CULL_MODE_FRONT_BIT, /**< Front-facing faces are culled. */
        BACK = VK_CULL_MODE_BACK_BIT, /**< Back-facing faces are culled. */
        FRONT_AND_BACK = VK_CULL_MODE_FRONT_AND_BACK /**< All faces are culled. */
    };

    /** @brief Vertex winding order determining front-facing polygons, mirroring `VkFrontFace`. */
    enum class FrontFace : uint32_t {
        COUNTER_CLOCKWISE = VK_FRONT_FACE_COUNTER_CLOCKWISE, /**< Counter-clockwise winding is considered front-facing. */
        CLOCKWISE = VK_FRONT_FACE_CLOCKWISE /**< Clockwise winding is considered front-facing. */
    };

    /** @brief Depth/Stencil comparison operations, mirroring `VkCompareOp`. */
    enum class CompareOperation : uint32_t {
        NEVER = VK_COMPARE_OP_NEVER, /**< Comparison always fails. */
        LESS = VK_COMPARE_OP_LESS, /**< Passes if the new value is less than the stored value. */
        EQUAL = VK_COMPARE_OP_EQUAL, /**< Passes if the new value equals the stored value. */
        LESS_OR_EQUAL = VK_COMPARE_OP_LESS_OR_EQUAL, /**< Passes if the new value is less than or equal to the stored value. */
        GREATER = VK_COMPARE_OP_GREATER, /**< Passes if the new value is greater than the stored value. */
        NOT_EQUAL = VK_COMPARE_OP_NOT_EQUAL, /**< Passes if the new value does not equal the stored value. */
        GREATER_OR_EQUAL = VK_COMPARE_OP_GREATER_OR_EQUAL, /**< Passes if the new value is greater than or equal to the stored value. */
        ALWAYS = VK_COMPARE_OP_ALWAYS /**< Comparison always passes. */
    };

    /** @brief Color blending factors for source and destination channels, mirroring `VkBlendFactor`. */
    enum class BlendFactor : uint32_t {
        ZERO = VK_BLEND_FACTOR_ZERO, /**< Factor of (0, 0, 0, 0). */
        ONE = VK_BLEND_FACTOR_ONE, /**< Factor of (1, 1, 1, 1). */
        SRC_COLOR = VK_BLEND_FACTOR_SRC_COLOR, /**< Factor equal to the source color. */
        ONE_MINUS_SRC_COLOR = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, /**< Factor equal to (1 - source color). */
        DST_COLOR = VK_BLEND_FACTOR_DST_COLOR, /**< Factor equal to the destination color. */
        ONE_MINUS_DST_COLOR = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR, /**< Factor equal to (1 - destination color). */
        SRC_ALPHA = VK_BLEND_FACTOR_SRC_ALPHA, /**< Factor equal to the source alpha. */
        ONE_MINUS_SRC_ALPHA = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, /**< Factor equal to (1 - source alpha). */
        DST_ALPHA = VK_BLEND_FACTOR_DST_ALPHA, /**< Factor equal to the destination alpha. */
        ONE_MINUS_DST_ALPHA = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA /**< Factor equal to (1 - destination alpha). */
    };

    /** @brief Arithmetic operations for color blending, mirroring `VkBlendOp`. */
    enum class BlendOperation : uint32_t {
        ADD = VK_BLEND_OP_ADD, /**< Source and destination contributions are added. */
        SUBTRACT = VK_BLEND_OP_SUBTRACT, /**< Destination contribution is subtracted from source. */
        REVERSE_SUBTRACT = VK_BLEND_OP_REVERSE_SUBTRACT, /**< Source contribution is subtracted from destination. */
        MIN = VK_BLEND_OP_MIN, /**< The minimum of source and destination is used. */
        MAX = VK_BLEND_OP_MAX /**< The maximum of source and destination is used. */
    };

    /**
     * @brief Comprehensive configuration structure for creating a `GraphicsPipeline`.
     */
    struct GraphicsPipelineCreateInfo {
        Shader *vertexShader = nullptr; /**< Required vertex shader stage */
        Shader *fragmentShader = nullptr; /**< Required fragment shader stage */

        PipelineLayout *layout = nullptr; /**< Required pipeline layout configuration */
        RenderPass *renderPass = nullptr; /**< Compatible render pass */
        VertexLayout *vertexLayout = nullptr; /**< Vertex input binding/attribute layout; leave `nullptr` for pipelines with no vertex input. */

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
     *
     * @details
     * Manages full graphics state setup including shaders, rasterization,
     * multisampling, depth/stencil, blending, and dynamic state bindings,
     * all configured through a single #GraphicsPipelineCreateInfo rather
     * than the half-dozen `VkGraphicsPipelineCreateInfo` sub-structures
     * Vulkan normally requires. `GraphicsPipeline` is move-only RAII: the
     * underlying `VkPipeline` is created in the constructor and destroyed
     * in the destructor.
     *
     * Example:
     * @code
     * LavaVK::GraphicsPipeline pipeline(device, {
     *     .vertexShader = &vertexShader,
     *     .fragmentShader = &fragmentShader,
     *     .layout = &pipelineLayout,
     *     .renderPass = &renderPass,
     *     .vertexLayout = &vertexLayout,
     *     .topology = LavaVK::Topology::TRIANGLES,
     *     .cullMode = LavaVK::CullMode::BACK,
     *     .depthTest = true,
     *     .depthWrite = true,
     * });
     *
     * cmd.bindPipeline(pipeline);
     * @endcode
     */
    class GraphicsPipeline {
    public:
        /**
         * @brief Constructs and compiles a native Vulkan graphics pipeline.
         * @param device Reference to the LavaVK logical device.
         * @param info Pipeline configuration descriptor.
         * @throws std::runtime_error If required parameters are missing or Vulkan pipeline compilation fails.
         */
        GraphicsPipeline(
            Device &device,
            const GraphicsPipelineCreateInfo &info);

        /**
         * @brief Destroys the underlying `VkPipeline`.
         * @details A moved-from `GraphicsPipeline` is a no-op.
         */
        ~GraphicsPipeline();

        // Non-copyable
        GraphicsPipeline(const GraphicsPipeline &) = delete;

        GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;

        // Moveable
        /**
         * @brief Move constructor. Transfers ownership of the underlying `VkPipeline`.
         * @param other The pipeline being moved from; left in an empty, destructible state.
         */
        GraphicsPipeline(GraphicsPipeline &&other) noexcept;

        /**
         * @brief Move assignment operator. Destroys any currently owned pipeline
         * and transfers ownership of @p other's `VkPipeline`.
         * @param other The pipeline being moved from; left in an empty, destructible state.
         * @return Reference to this pipeline.
         */
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

    /**
     * @brief Pipeline stage bitmask used for synchronization, mirroring `VkPipelineStageFlagBits`.
     * @details Used to specify at which stage(s) of the pipeline a wait
     * semaphore applies when submitting work (see `Device::submit`).
     * Multiple stages may be combined with the bitwise OR operator.
     */
    enum class PipelineStage : VkPipelineStageFlags {
        None = 0, /**< No stages. */
        TopOfPipe = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, /**< The very start of the pipeline. */
        DrawIndirect = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, /**< Indirect draw/dispatch command reads. */
        VertexInput = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, /**< Vertex/index buffer reads. */
        VertexShader = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, /**< Vertex shader execution. */
        TessellationControl = VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, /**< Tessellation control shader execution. */
        TessellationEvaluation = VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, /**< Tessellation evaluation shader execution. */
        GeometryShader = VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, /**< Geometry shader execution. */
        FragmentShader = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, /**< Fragment shader execution. */
        EarlyFragmentTests = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, /**< Depth/stencil tests before fragment shading. */
        LateFragmentTests = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, /**< Depth/stencil tests after fragment shading. */
        ColorAttachmentOutput = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, /**< Color attachment writes; the typical wait stage for swapchain image availability. */
        ComputeShader = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, /**< Compute shader execution. */
        Transfer = VK_PIPELINE_STAGE_TRANSFER_BIT, /**< Copy/blit/clear transfer commands. */
        BottomOfPipe = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, /**< The very end of the pipeline. */
        Host = VK_PIPELINE_STAGE_HOST_BIT, /**< Host (CPU) memory access. */
        AllGraphics = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, /**< All graphics pipeline stages. */
        AllCommands = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT /**< All command-processing stages. */
    };

    /**
     * @brief Combines two PipelineStage flags.
     * @param lhs First set of pipeline stage flags.
     * @param rhs Second set of pipeline stage flags.
     * @return A #PipelineStage containing the bitwise OR of @p lhs and @p rhs.
     */
    inline PipelineStage operator|(PipelineStage lhs, PipelineStage rhs) {
        return static_cast<PipelineStage>(
            static_cast<VkPipelineStageFlags>(lhs) | static_cast<VkPipelineStageFlags>(rhs)
        );
    }

    /**
     * @brief Selects which pipeline type a bind point operation (e.g. descriptor
     * set binding) applies to, mirroring `VkPipelineBindPoint`.
     */
    enum class PipelineBindPoint{
        Graphics = VK_PIPELINE_BIND_POINT_GRAPHICS, /**< Binds to the graphics pipeline bind point. */
        Compute = VK_PIPELINE_BIND_POINT_COMPUTE, /**< Binds to the compute pipeline bind point. */
    };

    /**
     * @brief Configuration structure for creating a `ComputePipeline`.
     */
    struct ComputePipelineCreateInfo {
        Shader *computeShader = nullptr; /**< Required compute shader stage. */
        PipelineLayout *layout = nullptr; /**< Required pipeline layout configuration. */
    };

    /**
     * @brief Abstraction around Vulkan's compute pipeline (`VkPipeline`).
     *
     * @details
     * Wraps the smaller subset of state needed to create a compute
     * pipeline (a single compute shader stage plus a `PipelineLayout`),
     * as opposed to the many rasterization/blend/depth states required by
     * `GraphicsPipeline`. `ComputePipeline` is move-only RAII: the
     * underlying `VkPipeline` is created in the constructor and destroyed
     * in the destructor.
     *
     * Example:
     * @code
     * LavaVK::ComputePipeline pipeline(device, {
     *     .computeShader = &computeShader,
     *     .layout = &pipelineLayout,
     * });
     *
     * cmd.bindPipeline(pipeline);
     * cmd.dispatch(groupCountX, groupCountY, groupCountZ);
     * @endcode
     */
    class ComputePipeline {
    public:
        /**
         * @brief Constructs and compiles a native Vulkan compute pipeline.
         * @param device Reference to the LavaVK logical device.
         * @param info Pipeline configuration descriptor (compute shader and layout).
         * @throws std::runtime_error If required parameters are missing or Vulkan pipeline compilation fails.
         */
        ComputePipeline(Device &device, const ComputePipelineCreateInfo &info);

        /**
         * @brief Destroys the underlying `VkPipeline`.
         * @details A moved-from `ComputePipeline` is a no-op.
         */
        ~ComputePipeline();

        // Non-copyable
        ComputePipeline(const ComputePipeline&) = delete;
        ComputePipeline& operator=(const ComputePipeline&) = delete;

        // Moveable
        /**
         * @brief Move constructor. Transfers ownership of the underlying `VkPipeline`.
         * @param other The pipeline being moved from; left in an empty, destructible state.
         */
        ComputePipeline(ComputePipeline&& other) noexcept;

        /**
         * @brief Move assignment operator. Destroys any currently owned pipeline
         * and transfers ownership of @p other's `VkPipeline`.
         * @param other The pipeline being moved from; left in an empty, destructible state.
         * @return Reference to this pipeline.
         */
        ComputePipeline& operator=(ComputePipeline&& other) noexcept;

        /**
         * @brief Binds this compute pipeline to a command buffer (`vkCmdBindPipeline`).
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


} // namespace LavaVK

#endif // LAVAVK_PIPELINE_HPP