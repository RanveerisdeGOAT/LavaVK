#include "LavaVK/Pipeline.hpp"
#include "LavaVK/Buffer.hpp"
#include "LavaVK/Descriptor.hpp"
#include "LavaVK/Instance.hpp"

namespace LavaVK {
    PipelineLayout::PipelineLayout(Device &device,
                                   const std::vector<const DescriptorSetLayout *> &descriptorSetLayouts,
                                   const std::vector<PushConstantRange> &pushConstantRanges)
        : m_device(device) {
        // Extract raw descriptor set layout handles
        std::vector<VkDescriptorSetLayout> nativeLayouts;
        nativeLayouts.reserve(descriptorSetLayouts.size());

        for (const auto *layout: descriptorSetLayouts) {
            if (layout != nullptr) {
                nativeLayouts.push_back(layout->native());
            }
        }

        // Convert PushConstantRange structs to VkPushConstantRange
        std::vector<VkPushConstantRange> vkPushConstantRanges;
        vkPushConstantRanges.reserve(pushConstantRanges.size());

        for (const auto &range: pushConstantRanges) {
            VkPushConstantRange vkRange{};
            vkRange.stageFlags = toVkShaderStageFlags(range.stageFlags);
            vkRange.offset = range.offset;
            vkRange.size = range.size;
            vkPushConstantRanges.push_back(vkRange);
        }

        VkPipelineLayoutCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        createInfo.setLayoutCount = static_cast<uint32_t>(nativeLayouts.size());
        createInfo.pSetLayouts = nativeLayouts.empty() ? nullptr : nativeLayouts.data();
        createInfo.pushConstantRangeCount = static_cast<uint32_t>(vkPushConstantRanges.size());
        createInfo.pPushConstantRanges = vkPushConstantRanges.empty() ? nullptr : vkPushConstantRanges.data();

        if (vkCreatePipelineLayout(m_device.native(), &createInfo, nullptr, &m_layout) != VK_SUCCESS) {
            LAVAVK_ERROR("[LavaVK ERROR] Failed to create pipeline layout!");
        }
    }

    PipelineLayout::~PipelineLayout() {
        if (m_layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_device.native(), m_layout, nullptr);
        }
    }


    RenderPass::RenderPass(Device &device, Format colorFormat, Format depthFormat, VkSampleCountFlagBits samples)
        : m_device(device), m_colorFormat(colorFormat.native()), m_depthFormat(depthFormat.native()) {
        // Color Attachment
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = colorFormat.native();
        colorAttachment.samples = samples;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Depth Attachment
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthFormat.native();
        depthAttachment.samples = samples;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Subpass
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = (!depthFormat.isUndefined()) ? &depthAttachmentRef : nullptr;

        // Subpass Dependency
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
        uint32_t attachmentCount = (depthFormat != VK_FORMAT_UNDEFINED) ? 2 : 1;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = attachmentCount;
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(m_device.native(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
            LAVAVK_ERROR("[LavaVK ERROR] Failed to create render pass!");
        }
    }

    RenderPass::RenderPass(Device &device, const VkRenderPassCreateInfo &createInfo)
        : m_device(device) {
        if (vkCreateRenderPass(m_device.native(), &createInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
            LAVAVK_ERROR("[LavaVK ERROR] Failed to create render pass!");
        }
    }

    RenderPass::~RenderPass() {
        if (m_renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(m_device.native(), m_renderPass, nullptr);
        }
    }

    GraphicsPipeline::GraphicsPipeline(
        Device &device,
        const GraphicsPipelineCreateInfo &info)
        : m_device(device) {
        if (!info.vertexShader)
            LAVAVK_ERROR("[LavaVK ERROR] Vertex shader is null.");

        if (!info.fragmentShader)
            LAVAVK_ERROR("[LavaVK ERROR] Fragment shader is null.");

        if (!info.layout)
            LAVAVK_ERROR("[LavaVK ERROR] PipelineLayout is null.");

        if (!info.renderPass)
            LAVAVK_ERROR("[LavaVK ERROR] RenderPass is null.");

        VkPipelineShaderStageCreateInfo shaderStages[2]{};

        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = info.vertexShader->native();
        shaderStages[0].pName = "main";

        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = info.fragmentShader->native();
        shaderStages[1].pName = "main";

        // --- Vertex Input State (POPULATED FROM info.vertexLayout) ---
        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        if (info.vertexLayout) {
            vertexInput.vertexBindingDescriptionCount =
                    static_cast<uint32_t>(info.vertexLayout->bindings().size());
            vertexInput.pVertexBindingDescriptions =
                    info.vertexLayout->bindings().data();

            vertexInput.vertexAttributeDescriptionCount =
                    static_cast<uint32_t>(info.vertexLayout->attributes().size());
            vertexInput.pVertexAttributeDescriptions =
                    info.vertexLayout->attributes().data();
        } else {
            vertexInput.vertexBindingDescriptionCount = 0;
            vertexInput.pVertexBindingDescriptions = nullptr;
            vertexInput.vertexAttributeDescriptionCount = 0;
            vertexInput.pVertexAttributeDescriptions = nullptr;
        }

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = static_cast<VkPrimitiveTopology>(info.topology);
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = static_cast<VkPolygonMode>(info.polygonMode);
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = static_cast<VkCullModeFlags>(info.cullMode);
        rasterizer.frontFace = static_cast<VkFrontFace>(info.frontFace);
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = info.samples;
        multisampling.sampleShadingEnable = VK_FALSE;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = info.depthTest ? VK_TRUE : VK_FALSE;
        depthStencil.depthWriteEnable = info.depthWrite ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp = static_cast<VkCompareOp>(info.depthCompare);
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = info.blending ? VK_TRUE : VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = static_cast<VkBlendFactor>(info.srcColorBlendFactor);
        colorBlendAttachment.dstColorBlendFactor = static_cast<VkBlendFactor>(info.dstColorBlendFactor);
        colorBlendAttachment.colorBlendOp = static_cast<VkBlendOp>(info.colorBlendOperation);
        colorBlendAttachment.srcAlphaBlendFactor = static_cast<VkBlendFactor>(info.srcAlphaBlendFactor);
        colorBlendAttachment.dstAlphaBlendFactor = static_cast<VkBlendFactor>(info.dstAlphaBlendFactor);
        colorBlendAttachment.alphaBlendOp = static_cast<VkBlendOp>(info.alphaBlendOperation);

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        std::vector<VkDynamicState> dynamicStates;
        if (info.dynamicViewport)
            dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
        if (info.dynamicScissor)
            dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInput; // Now correctly points to populated vertexInput!
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;

        pipelineInfo.layout = info.layout->native();
        pipelineInfo.renderPass = info.renderPass->native();
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(
                m_device.native(),
                VK_NULL_HANDLE,
                1,
                &pipelineInfo,
                nullptr,
                &m_pipeline) != VK_SUCCESS) {
            LAVAVK_ERROR("[LavaVK ERROR] Failed to create graphics pipeline.");
        }
    }

    GraphicsPipeline::~GraphicsPipeline() {
        if (m_pipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(m_device.native(), m_pipeline, nullptr);
    }

    GraphicsPipeline::GraphicsPipeline(GraphicsPipeline &&other) noexcept
        : m_device(other.m_device),
          m_pipeline(other.m_pipeline) {
        other.m_pipeline = VK_NULL_HANDLE;
    }

    GraphicsPipeline &GraphicsPipeline::operator=(GraphicsPipeline &&other) noexcept {
        if (this != &other) {
            if (m_pipeline != VK_NULL_HANDLE)
                vkDestroyPipeline(m_device.native(), m_pipeline, nullptr);

            m_pipeline = other.m_pipeline;
            other.m_pipeline = VK_NULL_HANDLE;
        }

        return *this;
    }

    void GraphicsPipeline::bind(VkCommandBuffer commandBuffer) const {
        vkCmdBindPipeline(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipeline);
    }
} // namespace LavaVK
