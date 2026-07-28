#include "LavaVK/Command.hpp"
#include "LavaVK/Pipeline.hpp"
#include <stdexcept>

#include "LavaVK/Buffer.hpp"

namespace LavaVK {
    void CommandBuffer::begin() const {
        VkCommandBufferBeginInfo beginInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        if (vkBeginCommandBuffer(m_buffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }
    }

    void CommandBuffer::end() const {
        if (vkEndCommandBuffer(m_buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to end recording command buffer!");
        }
    }

    void CommandBuffer::reset() const {
        if (vkResetCommandBuffer(m_buffer, 0) != VK_SUCCESS) {
            throw std::runtime_error("Failed to reset command buffer!");
        }
    }

    void CommandBuffer::bindPipeline(const GraphicsPipeline &pipeline) const {
        vkCmdBindPipeline(m_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.native());
    }

    void CommandBuffer::bindVertexBuffer(VkBuffer buffer, VkDeviceSize offset) const {
        VkBuffer buffers[] = {buffer};
        VkDeviceSize offsets[] = {offset};
        vkCmdBindVertexBuffers(m_buffer, 0, 1, buffers, offsets);
    }

    void CommandBuffer::bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) const {
        vkCmdBindIndexBuffer(m_buffer, buffer, offset, indexType);
    }

    void CommandBuffer::beginRenderPass(const VkRenderPassBeginInfo &renderPassInfo, VkSubpassContents contents) const {
        vkCmdBeginRenderPass(m_buffer, &renderPassInfo, contents);
    }

    void CommandBuffer::beginRenderPass(
        const RenderPass& renderPass,
        const Framebuffer& framebuffer,
        VkExtent2D extent,
        const std::array<float, 4>& clearColor,
        float clearDepth,
        uint32_t clearStencil,
        bool autoSetViewportAndScissor
    ) const {
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{clearColor[0], clearColor[1], clearColor[2], clearColor[3]}};
        clearValues[1].depthStencil = {clearDepth, clearStencil};

        VkRenderPassBeginInfo renderPassInfo{.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        renderPassInfo.renderPass = renderPass.native();
        renderPassInfo.framebuffer = framebuffer.native();
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = extent;
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(m_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        if (autoSetViewportAndScissor) setViewportAndScissor(extent);
    }

    void CommandBuffer::record(
        const RenderPass& renderPass,
        const Framebuffer& framebuffer,
        VkExtent2D extent,
        const std::function<void(CommandBuffer&)>& drawCommands)
    {
        reset();
        begin();
        beginRenderPass(renderPass, framebuffer, extent);

        // Execute draw calls provided by caller
        drawCommands(*this);

        endRenderPass();
        end();
    }

    void CommandBuffer::endRenderPass() const {
        vkCmdEndRenderPass(m_buffer);
    }

    void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                             uint32_t firstInstance) const {
        vkCmdDraw(m_buffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                    int32_t vertexOffset, uint32_t firstInstance) const {
        vkCmdDrawIndexed(m_buffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void CommandBuffer::setViewport(VkExtent2D extent, float minDepth, float maxDepth) const {
        setViewport(static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 0.0f, minDepth, maxDepth);
    }

    void CommandBuffer::setViewport(float width, float height, float x, float y, float minDepth, float maxDepth) const {
        VkViewport viewport{
            .x = x,
            .y = y,
            .width = width,
            .height = height,
            .minDepth = minDepth,
            .maxDepth = maxDepth
        };
        vkCmdSetViewport(m_buffer, 0, 1, &viewport);
    }

    void CommandBuffer::setScissor(VkExtent2D extent, VkOffset2D offset) const {
        VkRect2D scissor{
            .offset = offset,
            .extent = extent
        };
        vkCmdSetScissor(m_buffer, 0, 1, &scissor);
    }

    void CommandBuffer::setViewportAndScissor(VkExtent2D extent) const {
        setViewport(extent);
        setScissor(extent);
    }

    CommandPool::CommandPool(
        Device &device,
        QueueType queueType,
        bool transient,
        bool resetIndividualBuffers)
        : m_device(device) {
        VkCommandPoolCreateInfo poolInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        poolInfo.queueFamilyIndex = m_device.getQueueFamily(queueType);

        VkCommandPoolCreateFlags flags = 0;
        if (transient) flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        if (resetIndividualBuffers) flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.flags = flags;

        if (vkCreateCommandPool(m_device.native(), &poolInfo, nullptr, &m_pool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool!");
        }
    }

    CommandPool::~CommandPool() {
        if (m_pool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_device.native(), m_pool, nullptr);
            m_pool = VK_NULL_HANDLE;
        }
    }

    CommandPool::CommandPool(CommandPool &&other) noexcept
        : m_device(other.m_device),
          m_pool(other.m_pool),
          m_allocatedBuffers(std::move(other.m_allocatedBuffers)) {
        other.m_pool = VK_NULL_HANDLE;
    }

    CommandPool &CommandPool::operator=(CommandPool &&other) noexcept {
        if (this != &other) {
            if (m_pool != VK_NULL_HANDLE) {
                vkDestroyCommandPool(m_device.native(), m_pool, nullptr);
            }
            m_pool = other.m_pool;
            m_allocatedBuffers = std::move(other.m_allocatedBuffers);
            other.m_pool = VK_NULL_HANDLE;
        }
        return *this;
    }

    CommandBuffer &CommandPool::allocate() {
        size_t startIndex = m_allocatedBuffers.size();
        allocate(1);
        return m_allocatedBuffers[startIndex];
    }

    std::vector<CommandBuffer> &CommandPool::allocate(uint32_t count) {
        std::vector<VkCommandBuffer> vkBuffers(count);

        VkCommandBufferAllocateInfo allocInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        allocInfo.commandPool = m_pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = count;

        if (vkAllocateCommandBuffers(m_device.native(), &allocInfo, vkBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers!");
        }

        m_allocatedBuffers.reserve(m_allocatedBuffers.size() + count);

        for (uint32_t i = 0; i < count; ++i) {
            CommandBuffer buf;
            buf.m_device = &m_device;
            buf.m_pool = m_pool;
            buf.m_buffer = vkBuffers[i];
            m_allocatedBuffers.push_back(buf);
        }

        return m_allocatedBuffers;
    }

    CommandBuffer &CommandPool::retrieve(size_t index) {
        if (index >= m_allocatedBuffers.size()) {
            throw std::out_of_range("CommandPool::retrieve - Index out of range!");
        }
        return m_allocatedBuffers[index];
    }

    const CommandBuffer &CommandPool::retrieve(size_t index) const {
        if (index >= m_allocatedBuffers.size()) {
            throw std::out_of_range("CommandPool::retrieve - Index out of range!");
        }
        return m_allocatedBuffers[index];
    }

    void CommandPool::reset() const {
        if (vkResetCommandPool(m_device.native(), m_pool, 0) != VK_SUCCESS) {
            throw std::runtime_error("Failed to reset command pool!");
        }
    }
} // namespace LavaVK
