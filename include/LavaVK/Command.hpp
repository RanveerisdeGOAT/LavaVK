#ifndef LAVAVK_COMMAND_HPP
#define LAVAVK_COMMAND_HPP

#include <stdexcept>
#include <vulkan/vulkan.h>
#include <array>
#include <functional>
#include <vector>

#include "Queue.hpp"
#include "Shader.hpp"
#include "Pipeline.hpp"

namespace LavaVK {
    class Buffer;
    class Framebuffer;
    class RenderPass;
    /**
     * @brief Wrapper around VkCommandBuffer for recording GPU rendering and compute instructions.
     *
     * @details
     * Command buffers are allocated from a CommandPool and submitted to a GPU queue for execution.
     * Members are marked const as recording commands modifies the GPU recording state pointed to
     * by the handle, rather than the C++ handle itself. `CommandBuffer` is a non-owning handle:
     * it is allocated and freed by its owning #CommandPool, so it has no destructor of its own to
     * manage and is freely copyable. Typical usage either records commands manually with
     * #begin()/#beginRenderPass()/.../#end(), or uses the higher-level #record() helper that
     * manages that whole lifecycle in one call.
     *
     * Example (manual recording):
     * @code
     * cmd.begin();
     * cmd.beginRenderPass(renderPass, framebuffer, extent);
     * cmd.bindPipeline(pipeline);
     * cmd.setViewportAndScissor(extent);
     * cmd.bindVertexBuffer(vertexBuffer);
     * cmd.bindIndexBuffer(indexBuffer);
     * cmd.drawIndexed(indexCount);
     * cmd.endRenderPass();
     * cmd.end();
     * @endcode
     *
     * Example (using #record()):
     * @code
     * cmd.record(renderPass, framebuffer, extent, [&](LavaVK::CommandBuffer &cmd) {
     *     cmd.bindPipeline(pipeline);
     *     cmd.setViewportAndScissor(extent);
     *     cmd.bindVertexBuffer(vertexBuffer);
     *     cmd.bindIndexBuffer(indexBuffer);
     *     cmd.drawIndexed(indexCount);
     * });
     * @endcode
     */
    class CommandBuffer {
    public:
        /**
         * @brief Constructs an uninitialized CommandBuffer handle.
         * @details `native()` returns `VK_NULL_HANDLE` until this handle is
         * populated by #CommandPool::allocate().
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
         * @details Wraps `vkCmdBindPipeline` with `VK_PIPELINE_BIND_POINT_GRAPHICS`.
         * Must be called between #begin() (and typically #beginRenderPass())
         * and #end() before recording any draw calls.
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
         * @details Low-level entry point for callers that need full control
         * over `VkRenderPassBeginInfo` (custom clear values, multiple
         * attachments, render area offsets, etc.); most callers can instead
         * use the #beginRenderPass(const RenderPass&, const Framebuffer&, VkExtent2D, ...)
         * convenience overload below.
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
            const Framebuffer &framebuffer,
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
            const RenderPass &renderPass,
            const Framebuffer &framebuffer,
            VkExtent2D extent,
            const std::function<void(CommandBuffer &)> &drawCommands);

        /**
         * @brief Ends the active render pass.
         * @details Wraps `vkCmdEndRenderPass`. Must be called after all draw
         * commands for the current render pass instance have been recorded,
         * and before #end().
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
         * @brief Pushes constants to a pipeline layout.
         * @details Wraps `vkCmdPushConstants`, uploading @p data as a fast,
         * small block of shader-visible memory (typically used for
         * per-draw values like a model-view-projection matrix). The push
         * constant range covering `[offset, offset + sizeof(T))` and
         * @p stageFlags must match a range declared on @p layout.
         * @tparam T Trivially-copyable type of the data being pushed; its size
         * (via `sizeof(T)`) determines how many bytes are uploaded.
         * @param layout The pipeline layout declaring the push constant range being written to.
         * @param stageFlags Shader stages that will be able to access the pushed data.
         * @param data The value to upload as push constant data.
         * @param offset Byte offset into the push constant range to start writing at.
         */
        template<typename T>
        void pushConstants(
            const PipelineLayout &layout,
            ShaderStageFlags stageFlags,
            const T &data,
            uint32_t offset = 0
        ) {
            vkCmdPushConstants(
                m_buffer,
                layout.native(),
                stageFlags,
                offset,
                sizeof(T),
                &data
            );
        }

        /**
         * @brief Bind a vertex buffer to a specific binding slot.
         * @param binding Vertex input binding slot to bind @p vertexBuffer to,
         * matching a binding declared in the pipeline's #VertexLayout.
         * @param vertexBuffer The LavaVK #Buffer containing vertex data.
         * @param offset Byte offset into @p vertexBuffer from which vertex reading starts.
         */
        void bindVertexBuffer(uint32_t binding, const Buffer &vertexBuffer, VkDeviceSize offset = 0) const;

        /**
         * @brief Convenience overload defaulting to binding 0.
         * @param vertexBuffer The LavaVK #Buffer containing vertex data.
         * @param offset Byte offset into @p vertexBuffer from which vertex reading starts.
         */
        void bindVertexBuffer(const Buffer &vertexBuffer, VkDeviceSize offset = 0) {
            bindVertexBuffer(0, vertexBuffer, offset);
        }

        /**
         * @brief Bind multiple vertex buffers at once starting at firstBinding.
         * @details Wraps `vkCmdBindVertexBuffers`, binding each buffer in
         * @p buffers to consecutive binding slots starting at @p firstBinding.
         * @param firstBinding First vertex input binding slot to bind to.
         * @param buffers Vertex buffers to bind, one per consecutive binding slot.
         * @param offsets Byte offsets into each corresponding buffer in @p buffers;
         * must be the same size as @p buffers.
         */
        void bindVertexBuffers(
            uint32_t firstBinding,
            const std::vector<const Buffer *> &buffers,
            const std::vector<VkDeviceSize> &offsets
        ) const;

        /**
         * @brief Binds an index buffer (defaults to VK_INDEX_TYPE_UINT16).
         * @param indexBuffer The LavaVK #Buffer containing index data.
         * @param indexType Type of indices stored in @p indexBuffer.
         * @param offset Byte offset into @p indexBuffer from which index reading starts.
         */
        void bindIndexBuffer(
            const Buffer &indexBuffer,
            VkIndexType indexType = VK_INDEX_TYPE_UINT16,
            VkDeviceSize offset = 0
        ) const;

        /**
         * @brief Returns the underlying Vulkan VkCommandBuffer handle.
         * @return The native `VkCommandBuffer` handle, or `VK_NULL_HANDLE` if unallocated.
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
     * @details
     * A CommandPool encapsulates a VkCommandPool bound to a specific QueueType.
     * It maintains ownership of all CommandBuffers allocated from it; individual
     * `CommandBuffer` handles are non-owning views into storage kept alive by
     * their `CommandPool`, and are invalidated when the pool is destroyed or
     * moved-from. `Device` normally owns one `CommandPool` per requested
     * `QueueType`, created lazily via `Device::getCommandPool()`, so
     * applications typically do not construct a `CommandPool` directly.
     *
     * Example, allocating one command buffer per frame in flight:
     * @code
     * LavaVK::CommandPool &pool = device.getCommandPool(LavaVK::QueueType::GRAPHICS);
     * pool.allocate(LavaVK::MAX_FRAMES_IN_FLIGHT);
     *
     * LavaVK::CommandBuffer &cmd = pool.retrieve(frameIndex);
     * @endcode
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
         * @details Calls `vkDestroyCommandPool`, which implicitly frees every
         * `VkCommandBuffer` allocated from it; a moved-from `CommandPool` is a no-op.
         */
        ~CommandPool();

        CommandPool(const CommandPool &) = delete;

        CommandPool &operator=(const CommandPool &) = delete;

        /**
         * @brief Move constructor for transfer of GPU command pool resource ownership.
         * @param other The command pool being moved from; left in an empty, destructible state.
         */
        CommandPool(CommandPool &&other) noexcept;

        /**
         * @brief Move assignment operator for transfer of GPU command pool resource ownership.
         * @param other The command pool being moved from; left in an empty, destructible state.
         * @return Reference to this command pool.
         */
        CommandPool &operator=(CommandPool &&other) noexcept;

        /**
         * @brief Allocates a single CommandBuffer from the pool and stores it in internal storage.
         * @return Reference to the newly allocated CommandBuffer.
         * @throw std::runtime_error If `vkAllocateCommandBuffers` fails.
         */
        CommandBuffer &allocate();

        /**
         * @brief Allocates multiple CommandBuffers from the pool in a single batch call.
         * @param count Number of command buffers to allocate.
         * @return Reference to the internal vector of allocated command buffers.
         * @throw std::runtime_error If `vkAllocateCommandBuffers` fails.
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
         * @details Wraps `vkResetCommandPool`, returning every command buffer
         * allocated from this pool to the initial (unrecorded) state without
         * freeing them.
         * @throws std::runtime_error If resetting the command pool fails.
         */
        void reset() const;

        /**
         * @brief Returns the total number of command buffers allocated from this pool.
         * @return Count of `CommandBuffer` entries currently allocated from this pool.
         */
        [[nodiscard]] size_t size() const { return m_allocatedBuffers.size(); }

        /**
         * @brief Returns the raw VkCommandPool handle.
         * @return The native `VkCommandPool` handle, or `VK_NULL_HANDLE` if moved-from.
         */
        [[nodiscard]] VkCommandPool native() const { return m_pool; }

    private:
        Device &m_device;
        VkCommandPool m_pool = VK_NULL_HANDLE;
        std::vector<CommandBuffer> m_allocatedBuffers;
    };
} // namespace LavaVK

#endif // LAVAVK_COMMAND_HPP