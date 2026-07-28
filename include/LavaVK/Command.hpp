#ifndef LAVAVK_COMMAND_HPP
#define LAVAVK_COMMAND_HPP

#include "Device.hpp"
#include <array>
#include <functional>
#include <vector>
#include <vulkan/vulkan.h>

namespace LavaVK {
    class Framebuffer;
    class RenderPass;
    class GraphicsPipeline;

    /**
     * @brief Wrapper around VkCommandBuffer for recording GPU rendering and compute instructions.
     *
     * Command buffers are allocated from a CommandPool and submitted to a GPU queue for execution.
     * Members are marked const as recording commands modifies the GPU recording state pointed to
     * by the handle, rather than the C++ handle itself.
     */
    class CommandBuffer {
    public:
        /**
         * @brief Constructs an uninitialized CommandBuffer handle.
         */
        CommandBuffer() = default;

        /**
         * @brief Begins recording commands into the command buffer.
         * @throws std::runtime_error If starting recording fails.
         */
        void begin() const;

        /**
         * @brief Ends command recording. Must be called before queue submission.
         * @throws std::runtime_error If ending recording fails.
         */
        void end() const;

        /**
         * @brief Clears all pre-recorded commands from the buffer.
         * @throws std::runtime_error If resetting the buffer fails.
         */
        void reset() const;

        /**
         * @brief Binds a graphics pipeline to the command buffer.
         * @param pipeline The GraphicsPipeline instance containing shader stages and pipeline state.
         */
        void bindPipeline(const GraphicsPipeline &pipeline) const;

        /**
         * @brief Binds a vertex buffer to the pipeline.
         * @param buffer Native handle to the Vulkan vertex buffer.
         * @param offset Byte offset into the buffer from which vertex reading starts.
         */
        void bindVertexBuffer(VkBuffer buffer, VkDeviceSize offset = 0) const;

        /**
         * @brief Binds an index buffer for indexed drawing commands.
         * @param buffer Native handle to the Vulkan index buffer.
         * @param offset Byte offset into the buffer.
         * @param indexType Type of indices stored in the buffer (e.g., VK_INDEX_TYPE_UINT16 or VK_INDEX_TYPE_UINT32).
         */
        void bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset = 0,
                             VkIndexType indexType = VK_INDEX_TYPE_UINT32) const;

        /**
         * @brief Begins a render pass using explicit Vulkan begin info.
         * @param renderPassInfo Native VkRenderPassBeginInfo detailing framebuffers, clear values, and render area.
         * @param contents Defines if instructions are recorded directly inline or via secondary command buffers.
         */
        void beginRenderPass(const VkRenderPassBeginInfo &renderPassInfo,
                             VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE) const;

        /**
         * @brief Convenience overload to begin a render pass with sensible default clear values.
         * @param renderPass The RenderPass object defining attachments and subpasses.
         * @param framebuffer Native VkFramebuffer target to draw into.
         * @param extent The width and height of the rendering area (typically matching swapchain extent).
         * @param clearColor RGBA clear colors for the color attachment.
         * @param clearDepth Depth clear value (default is 1.0f).
         * @param clearStencil Stencil clear value (default is 0).
         * @param autoSetViewportAndScissor Automatically sets viewport and scissor states to match extent.
         */
        void beginRenderPass(
            const RenderPass &renderPass,
            const Framebuffer& framebuffer,
            VkExtent2D extent,
            const std::array<float, 4> &clearColor = {0.1f, 0.1f, 0.1f, 1.0f},
            float clearDepth = 1.0f,
            uint32_t clearStencil = 0,
            bool autoSetViewportAndScissor = true
        ) const;

        /**
         * @brief Higher-level helper that manages the entire lifecycle of recording a render pass.
         *
         * Automatically handles reset, begin, beginRenderPass, executing the draw callback,
         * endRenderPass, and end in one block.
         *
         * @param renderPass The RenderPass to record into.
         * @param framebuffer Target framebuffer.
         * @param extent Rendering extent.
         * @param drawCommands Lambda callback containing draw calls and pipeline bindings.
         */
        void record(
        const RenderPass& renderPass,
        const Framebuffer& framebuffer,
        VkExtent2D extent,
        const std::function<void(CommandBuffer &)> &drawCommands);

        /**
         * @brief Ends the active render pass.
         */
        void endRenderPass() const;

        /**
         * @brief Records a non-indexed draw command.
         * @param vertexCount Number of vertices to draw.
         * @param instanceCount Number of instances to draw (default 1).
         * @param firstVertex Index of the first vertex to draw.
         * @param firstInstance Instance ID of the first instance.
         */
        void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0,
                  uint32_t firstInstance = 0) const;

        /**
         * @brief Records an indexed draw command.
         * @param indexCount Number of indices to draw.
         * @param instanceCount Number of instances to draw (default 1).
         * @param firstIndex Byte offset index in the index buffer.
         * @param vertexOffset Value added to vertex indices before indexing into the vertex buffer.
         * @param firstInstance Instance ID of the first instance.
         */
        void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0,
                         int32_t vertexOffset = 0, uint32_t firstInstance = 0) const;

        /**
         * @brief Returns the underlying Vulkan VkCommandBuffer handle.
         */
        [[nodiscard]] VkCommandBuffer native() const { return m_buffer; }

        /**
         * @brief Sets dynamic viewport dimensions using a VkExtent2D.
         * @param extent Width and height of the viewport.
         * @param minDepth Minimum depth value range (0.0f to 1.0f).
         * @param maxDepth Maximum depth value range (0.0f to 1.0f).
         */
        void setViewport(VkExtent2D extent, float minDepth = 0.0f, float maxDepth = 1.0f) const;

        /**
         * @brief Sets dynamic viewport dimensions explicitly.
         * @param width Viewport width.
         * @param height Viewport height.
         * @param x Upper-left X coordinate.
         * @param y Upper-left Y coordinate.
         * @param minDepth Minimum depth value range (0.0f to 1.0f).
         * @param maxDepth Maximum depth value range (0.0f to 1.0f).
         */
        void setViewport(float width, float height, float x = 0.0f, float y = 0.0f, float minDepth = 0.0f,
                         float maxDepth = 1.0f) const;

        /**
         * @brief Sets the dynamic scissor rectangle.
         * @param extent Width and height of the scissor rectangle.
         * @param offset Upper-left coordinate offset.
         */
        void setScissor(VkExtent2D extent, VkOffset2D offset = {0, 0}) const;

        /**
         * @brief Convenience function to configure both viewport and scissor rectangle simultaneously.
         * @param extent Extent applied to both viewport dimensions and scissor rectangle.
         */
        void setViewportAndScissor(VkExtent2D extent) const;

    private:
        friend class CommandPool;

        Device *m_device = nullptr;
        VkCommandPool m_pool = VK_NULL_HANDLE;
        VkCommandBuffer m_buffer = VK_NULL_HANDLE;
    };

    /**
     * @brief Opaque container managing GPU memory pools for command buffer allocation.
     *
     * A CommandPool encapsulates a VkCommandPool bound to a specific QueueType.
     * It maintains ownership of all CommandBuffers allocated from it.
     */
    class CommandPool {
    public:
        /**
         * @brief Creates a Vulkan command pool bound to the device's queue family.
         * @param device Reference to the logical Device.
         * @param queueType Queue family type (e.g. GRAPHICS, TRANSFER, COMPUTE).
         * @param transient Set true if command buffers allocated from this pool are short-lived.
         * @param resetIndividualBuffers Set true to allow command buffers to be reset independently.
         */
        CommandPool(
            Device &device,
            QueueType queueType,
            bool transient = false,
            bool resetIndividualBuffers = false);

        /**
         * @brief Destroys the Vulkan command pool and frees all allocated command buffers.
         */
        ~CommandPool();

        CommandPool(const CommandPool &) = delete;

        CommandPool &operator=(const CommandPool &) = delete;

        /**
         * @brief Move constructor for transfer of GPU command pool resource ownership.
         */
        CommandPool(CommandPool &&other) noexcept;

        /**
         * @brief Move assignment operator for transfer of GPU command pool resource ownership.
         */
        CommandPool &operator=(CommandPool &&other) noexcept;

        /**
         * @brief Allocates a single CommandBuffer from the pool and stores it in internal storage.
         * @return Reference to the newly allocated CommandBuffer.
         */
        CommandBuffer &allocate();

        /**
         * @brief Allocates multiple CommandBuffers from the pool in a single batch call.
         * @param count Number of command buffers to allocate.
         * @return Reference to the internal vector of allocated command buffers.
         */
        std::vector<CommandBuffer> &allocate(uint32_t count);

        /**
         * @brief Retrieves a reference to an allocated CommandBuffer by index.
         * @param index Index in internal vector storage (0 to size() - 1).
         * @return Reference to the allocated CommandBuffer.
         * @throws std::out_of_range If index exceeds size().
         */
        [[nodiscard]] CommandBuffer &retrieve(size_t index);

        /**
         * @brief Retrieves a const reference to an allocated CommandBuffer by index.
         * @param index Index in internal vector storage (0 to size() - 1).
         * @return Const reference to the allocated CommandBuffer.
         * @throws std::out_of_range If index exceeds size().
         */
        [[nodiscard]] const CommandBuffer &retrieve(size_t index) const;

        /**
         * @brief Resets the underlying Vulkan command pool and clears pre-recorded states.
         * @throws std::runtime_error If resetting the command pool fails.
         */
        void reset() const;

        /**
         * @brief Returns the total number of command buffers allocated from this pool.
         */
        [[nodiscard]] size_t size() const { return m_allocatedBuffers.size(); }

        /**
         * @brief Returns the raw VkCommandPool handle.
         */
        [[nodiscard]] VkCommandPool native() const { return m_pool; }

    private:
        Device &m_device;
        VkCommandPool m_pool = VK_NULL_HANDLE;
        std::vector<CommandBuffer> m_allocatedBuffers;
    };
} // namespace LavaVK

#endif // LAVAVK_COMMAND_HPP
