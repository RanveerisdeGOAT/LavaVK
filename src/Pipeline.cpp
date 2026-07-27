#include "include/LavaVK/Pipeline.hpp"

#include <fstream>
#include <vector>
#include <array>
#include <memory>
#include <stdexcept>
#include <unordered_map>

#include "LavaVK/Device.hpp"

namespace LavaVK {

    namespace {
        static std::vector<char> readFile(const std::string& filename) {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("[LavaVK ERROR] Failed to open shader file: " + filename);
            }

            size_t fileSize = static_cast<size_t>(file.tellg());
            std::vector<char> buffer(fileSize);
            file.seekg(0);
            file.read(buffer.data(), fileSize);
            file.close();

            return buffer;
        }

        static VkDescriptorType toVkDescriptorType(DescriptorType type) {
            switch (type) {
                case DescriptorType::Sampler:              return VK_DESCRIPTOR_TYPE_SAMPLER;
                case DescriptorType::CombinedImageSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                case DescriptorType::SampledImage:         return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                case DescriptorType::StorageImage:         return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                case DescriptorType::UniformTexelBuffer:   return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                case DescriptorType::StorageTexelBuffer:   return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                case DescriptorType::UniformBuffer:        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                case DescriptorType::StorageBuffer:        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                case DescriptorType::UniformBufferDynamic: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                case DescriptorType::StorageBufferDynamic: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
                case DescriptorType::InputAttachment:     return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            }
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        }

        static VkShaderStageFlags toVkShaderStageFlags(uint32_t flags) {
            VkShaderStageFlags result = 0;
            if (flags & STAGE_VERTEX_BIT)                  result |= VK_SHADER_STAGE_VERTEX_BIT;
            if (flags & STAGE_TESSELLATION_CONTROL_BIT)   result |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            if (flags & STAGE_TESSELLATION_EVALUATION_BIT) result |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            if (flags & STAGE_GEOMETRY_BIT)               result |= VK_SHADER_STAGE_GEOMETRY_BIT;
            if (flags & STAGE_FRAGMENT_BIT)               result |= VK_SHADER_STAGE_FRAGMENT_BIT;
            if (flags & STAGE_COMPUTE_BIT)                result |= VK_SHADER_STAGE_COMPUTE_BIT;
            return result;
        }
    }



    Shader::Shader(Device& device, const std::string& filepath)
        : m_device(device)
    {
        auto code = readFile(filepath);
        createShaderModule(code);
    }

    Shader::Shader(Device& device, const std::vector<char>& code)
        : m_device(device)
    {
        createShaderModule(code);
    }

    Shader::~Shader() {
        if (m_module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_device.native(), m_module, nullptr);
        }
    }

    void Shader::createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

        if (vkCreateShaderModule(m_device.native(), &createInfo, nullptr, &m_module) != VK_SUCCESS) {
            throw std::runtime_error("[LavaVK ERROR] Failed to create shader module!");
        }
    }



    PipelineLayout::PipelineLayout(Device& device,
                                   const std::vector<const DescriptorSetLayout*>& descriptorSetLayouts,
                                   const std::vector<PushConstantRange>& pushConstantRanges)
        : m_device(device)
    {
        // Extract raw descriptor set layout handles
        std::vector<VkDescriptorSetLayout> nativeLayouts;
        nativeLayouts.reserve(descriptorSetLayouts.size());

        for (const auto* layout : descriptorSetLayouts) {
            if (layout != nullptr) {
                nativeLayouts.push_back(layout->native());
            }
        }

        // Convert PushConstantRange structs to VkPushConstantRange
        std::vector<VkPushConstantRange> vkPushConstantRanges;
        vkPushConstantRanges.reserve(pushConstantRanges.size());

        for (const auto& range : pushConstantRanges) {
            VkPushConstantRange vkRange{};
            vkRange.stageFlags = toVkShaderStageFlags(range.stageFlags);
            vkRange.offset     = range.offset;
            vkRange.size       = range.size;
            vkPushConstantRanges.push_back(vkRange);
        }

        VkPipelineLayoutCreateInfo createInfo{};
        createInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        createInfo.setLayoutCount         = static_cast<uint32_t>(nativeLayouts.size());
        createInfo.pSetLayouts            = nativeLayouts.empty() ? nullptr : nativeLayouts.data();
        createInfo.pushConstantRangeCount = static_cast<uint32_t>(vkPushConstantRanges.size());
        createInfo.pPushConstantRanges    = vkPushConstantRanges.empty() ? nullptr : vkPushConstantRanges.data();

        if (vkCreatePipelineLayout(m_device.native(), &createInfo, nullptr, &m_layout) != VK_SUCCESS) {
            throw std::runtime_error("[LavaVK ERROR] Failed to create pipeline layout!");
        }
    }

    PipelineLayout::~PipelineLayout() {
        if (m_layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_device.native(), m_layout, nullptr);
        }
    }



    RenderPass::RenderPass(Device& device, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits samples)
        : m_device(device)
    {
        // Color Attachment
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format         = colorFormat;
        colorAttachment.samples        = samples;
        colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Depth Attachment
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format         = depthFormat;
        depthAttachment.samples        = samples;
        depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Subpass
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = (depthFormat != VK_FORMAT_UNDEFINED) ? &depthAttachmentRef : nullptr;

        // Subpass Dependency
        VkSubpassDependency dependency{};
        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
        uint32_t attachmentCount = (depthFormat != VK_FORMAT_UNDEFINED) ? 2 : 1;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = attachmentCount;
        renderPassInfo.pAttachments    = attachments.data();
        renderPassInfo.subpassCount    = 1;
        renderPassInfo.pSubpasses      = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies   = &dependency;

        if (vkCreateRenderPass(m_device.native(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
            throw std::runtime_error("[LavaVK ERROR] Failed to create render pass!");
        }
    }

    RenderPass::RenderPass(Device& device, const VkRenderPassCreateInfo& createInfo)
        : m_device(device)
    {
        if (vkCreateRenderPass(m_device.native(), &createInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
            throw std::runtime_error("[LavaVK ERROR] Failed to create render pass!");
        }
    }

    RenderPass::~RenderPass() {
        if (m_renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(m_device.native(), m_renderPass, nullptr);
        }
    }



    DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::addBinding(
        uint32_t binding,
        DescriptorType descriptorType,
        uint32_t stageFlags,
        uint32_t count)
    {
        DescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding         = binding;
        layoutBinding.descriptorType  = descriptorType;
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags      = stageFlags;

        m_bindings[binding] = layoutBinding;
        return *this;
    }

    std::unique_ptr<DescriptorSetLayout> DescriptorSetLayout::Builder::build() const {
        return std::make_unique<DescriptorSetLayout>(m_device, m_bindings);
    }

    DescriptorSetLayout::DescriptorSetLayout(Device& device, std::unordered_map<uint32_t, DescriptorSetLayoutBinding> bindings)
        : m_device(device), m_bindings(bindings)
    {
        std::vector<VkDescriptorSetLayoutBinding> vkBindings;
        vkBindings.reserve(m_bindings.size());

        for (const auto& [bindingIndex, binding] : m_bindings) {
            VkDescriptorSetLayoutBinding vkBinding{};
            vkBinding.binding            = binding.binding;
            vkBinding.descriptorType     = toVkDescriptorType(binding.descriptorType);
            vkBinding.descriptorCount    = binding.descriptorCount;
            vkBinding.stageFlags         = toVkShaderStageFlags(binding.stageFlags);
            vkBinding.pImmutableSamplers = nullptr;

            vkBindings.push_back(vkBinding);
        }

        VkDescriptorSetLayoutCreateInfo createInfo{};
        createInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.bindingCount = static_cast<uint32_t>(vkBindings.size());
        createInfo.pBindings    = vkBindings.data();

        if (vkCreateDescriptorSetLayout(m_device.native(), &createInfo, nullptr, &m_layout) != VK_SUCCESS) {
            throw std::runtime_error("[LavaVK ERROR] Failed to create descriptor set layout!");
        }
    }

    DescriptorSetLayout::~DescriptorSetLayout() {
        if (m_layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_device.native(), m_layout, nullptr);
        }
    }



    DescriptorPool::Builder& DescriptorPool::Builder::addPoolSize(DescriptorType descriptorType, uint32_t count) {
        m_poolCounts[descriptorType] += count;
        return *this;
    }

    DescriptorPool::Builder& DescriptorPool::Builder::setPoolFlags(VkDescriptorPoolCreateFlags flags) {
        m_poolFlags = flags;
        return *this;
    }

    DescriptorPool::Builder& DescriptorPool::Builder::setMaxSets(uint32_t count) {
        m_maxSets = count;
        return *this;
    }

    std::unique_ptr<DescriptorPool> DescriptorPool::Builder::build() const {
        std::vector<VkDescriptorPoolSize> poolSizes;
        poolSizes.reserve(m_poolCounts.size());

        for (const auto& [type, count] : m_poolCounts) {
            poolSizes.push_back({ toVkDescriptorType(type), count });
        }

        return std::make_unique<DescriptorPool>(m_device, m_maxSets, m_poolFlags, poolSizes);
    }

    DescriptorPool::DescriptorPool(
        Device& device,
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags flags,
        const std::vector<VkDescriptorPoolSize>& poolSizes)
        : m_device(device)
    {
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes    = poolSizes.data();
        poolInfo.maxSets       = maxSets;
        poolInfo.flags         = flags;

        if (vkCreateDescriptorPool(m_device.native(), &poolInfo, nullptr, &m_pool) != VK_SUCCESS) {
            throw std::runtime_error("[LavaVK ERROR] Failed to create descriptor pool!");
        }
    }

    DescriptorPool::~DescriptorPool() {
        if (m_pool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_device.native(), m_pool, nullptr);
        }
    }

    bool DescriptorPool::allocateDescriptorSet(VkDescriptorSetLayout layout, VkDescriptorSet& descriptorSet) const {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = m_pool;
        allocInfo.pSetLayouts        = &layout;
        allocInfo.descriptorSetCount = 1;

        return vkAllocateDescriptorSets(m_device.native(), &allocInfo, &descriptorSet) == VK_SUCCESS;
    }

    void DescriptorPool::freeDescriptorSets(const std::vector<VkDescriptorSet>& descriptorSets) const {
        vkFreeDescriptorSets(m_device.native(), m_pool, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data());
    }



    DescriptorWriter::DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool)
        : m_setLayout(setLayout), m_pool(pool) {}

    DescriptorWriter& DescriptorWriter::writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo) {
        const auto& bindings = m_setLayout.getBindings();
        auto bindingDescription = bindings.at(binding);

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType  = toVkDescriptorType(bindingDescription.descriptorType);
        write.dstBinding      = binding;
        write.pBufferInfo     = bufferInfo;
        write.descriptorCount = bindingDescription.descriptorCount;

        m_writes.push_back(write);
        return *this;
    }

    DescriptorWriter& DescriptorWriter::writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo) {
        const auto& bindings = m_setLayout.getBindings();
        auto bindingDescription = bindings.at(binding);

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType  = toVkDescriptorType(bindingDescription.descriptorType);
        write.dstBinding      = binding;
        write.pImageInfo      = imageInfo;
        write.descriptorCount = bindingDescription.descriptorCount;

        m_writes.push_back(write);
        return *this;
    }

    bool DescriptorWriter::build(VkDescriptorSet& set) {
        bool success = m_pool.allocateDescriptorSet(m_setLayout.native(), set);
        if (!success) {
            return false;
        }
        overwrite(set);
        return true;
    }

    void DescriptorWriter::overwrite(VkDescriptorSet& set) {
        for (auto& write : m_writes) {
            write.dstSet = set;
        }
        vkUpdateDescriptorSets(m_pool.m_device.native(), static_cast<uint32_t>(m_writes.size()), m_writes.data(), 0, nullptr);
    }

} // namespace LavaVK