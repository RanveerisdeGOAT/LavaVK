#include "include/LavaVK/Pipeline.hpp"

#include <fstream>
#include <vector>
#include <array>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <shaderc/shaderc.hpp>

#include "LavaVK/Device.hpp"

namespace LavaVK {
    namespace {
        static std::vector<uint32_t> readSpvFile(const std::string &filename) {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("[LavaVK ERROR] Failed to open shader file: " + filename);
            }

            size_t fileSize = static_cast<size_t>(file.tellg());
            if (fileSize % sizeof(uint32_t) != 0) {
                throw std::runtime_error(
                    "[LavaVK ERROR] Invalid SPIR-V file size (must be a multiple of 4 bytes): " + filename);
            }

            std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
            file.seekg(0);
            file.read(reinterpret_cast<char *>(buffer.data()), fileSize);
            file.close();

            return buffer;
        }

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

        static VkShaderStageFlags toVkShaderStageFlags(uint32_t flags) {
            VkShaderStageFlags result = 0;
            if (flags & STAGE_VERTEX_BIT) result |= VK_SHADER_STAGE_VERTEX_BIT;
            if (flags & STAGE_TESSELLATION_CONTROL_BIT) result |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            if (flags & STAGE_TESSELLATION_EVALUATION_BIT) result |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            if (flags & STAGE_GEOMETRY_BIT) result |= VK_SHADER_STAGE_GEOMETRY_BIT;
            if (flags & STAGE_FRAGMENT_BIT) result |= VK_SHADER_STAGE_FRAGMENT_BIT;
            if (flags & STAGE_COMPUTE_BIT) result |= VK_SHADER_STAGE_COMPUTE_BIT;
            return result;
        }

        // Helper to deduce ShaderType from file extension
        LavaVK::ShaderType deduceShaderType(const std::string &filepath) {
            std::filesystem::path path(filepath);
            std::string ext = path.extension().string();

            if (ext == ".vert") return LavaVK::ShaderType::Vertex;
            if (ext == ".frag") return LavaVK::ShaderType::Fragment;
            if (ext == ".comp") return LavaVK::ShaderType::Compute;

            throw std::runtime_error("[LavaVK Error] Unknown shader extension: " + ext);
        }

        std::vector<uint32_t> compileGLSLToSPIRV(
            const std::string &filepath,
            LavaVK::ShaderType type) {
            // Read source code from file
            std::ifstream file(filepath);
            if (!file.is_open()) {
                throw std::runtime_error("[Shaderc Error] Failed to open GLSL file: " + filepath);
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string sourceCode = buffer.str();

            shaderc::Compiler compiler;
            shaderc::CompileOptions options;

            options.SetOptimizationLevel(shaderc_optimization_level_performance);
            options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);

            shaderc_shader_kind kind;
            switch (type) {
                case LavaVK::ShaderType::Vertex: kind = shaderc_glsl_vertex_shader;
                    break;
                case LavaVK::ShaderType::Fragment: kind = shaderc_glsl_fragment_shader;
                    break;
                case LavaVK::ShaderType::Compute: kind = shaderc_glsl_compute_shader;
                    break;
            }

            shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
                sourceCode,
                kind,
                filepath.c_str(),
                options
            );

            if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
                throw std::runtime_error("[Shaderc Error] " + module.GetErrorMessage());
            }

            return {module.cbegin(), module.cend()};
        }
    }


    Shader::Shader(Device &device, const std::string &filepath)
        : m_device(device) {
        std::filesystem::path path(filepath);

        if (path.extension() == ".spv") {
            std::vector<uint32_t> spirv = readSpvFile(filepath);
            createShaderModule(spirv);
        } else {
            ShaderType type = deduceShaderType(filepath);
            std::vector<uint32_t> spirv = compileGLSLToSPIRV(filepath, type);
            createShaderModule(spirv);
        }
    }

    Shader::Shader(Device &device, const std::vector<uint32_t> &code)
        : m_device(device) {
        createShaderModule(code);
    }

    Shader::~Shader() {
        if (m_module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_device.native(), m_module, nullptr);
        }
    }

    void Shader::createShaderModule(const std::vector<uint32_t> &code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        // codeSize MUST BE IN BYTES
        createInfo.codeSize = code.size() * sizeof(uint32_t);
        // Properly aligned pointer
        createInfo.pCode = code.data();

        if (vkCreateShaderModule(m_device.native(), &createInfo, nullptr, &m_module) != VK_SUCCESS) {
            throw std::runtime_error("[LavaVK ERROR] Failed to create shader module!");
        }
    }


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
            throw std::runtime_error("[LavaVK ERROR] Failed to create pipeline layout!");
        }
    }

    PipelineLayout::~PipelineLayout() {
        if (m_layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_device.native(), m_layout, nullptr);
        }
    }


    RenderPass::RenderPass(Device &device, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits samples)
        : m_device(device) {
        // Color Attachment
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = colorFormat;
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
        depthAttachment.format = depthFormat;
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
        subpass.pDepthStencilAttachment = (depthFormat != VK_FORMAT_UNDEFINED) ? &depthAttachmentRef : nullptr;

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
            throw std::runtime_error("[LavaVK ERROR] Failed to create render pass!");
        }
    }

    RenderPass::RenderPass(Device &device, const VkRenderPassCreateInfo &createInfo)
        : m_device(device) {
        if (vkCreateRenderPass(m_device.native(), &createInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
            throw std::runtime_error("[LavaVK ERROR] Failed to create render pass!");
        }
    }

    RenderPass::~RenderPass() {
        if (m_renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(m_device.native(), m_renderPass, nullptr);
        }
    }


    DescriptorSetLayout::Builder &DescriptorSetLayout::Builder::addBinding(
        uint32_t binding,
        DescriptorType descriptorType,
        uint32_t stageFlags,
        uint32_t count) {
        DescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = stageFlags;

        m_bindings[binding] = layoutBinding;
        return *this;
    }

    std::unique_ptr<DescriptorSetLayout> DescriptorSetLayout::Builder::build() const {
        return std::make_unique<DescriptorSetLayout>(m_device, m_bindings);
    }

    DescriptorSetLayout::DescriptorSetLayout(Device &device,
                                             std::unordered_map<uint32_t, DescriptorSetLayoutBinding> bindings)
        : m_device(device), m_bindings(bindings) {
        std::vector<VkDescriptorSetLayoutBinding> vkBindings;
        vkBindings.reserve(m_bindings.size());

        for (const auto &[bindingIndex, binding]: m_bindings) {
            VkDescriptorSetLayoutBinding vkBinding{};
            vkBinding.binding = binding.binding;
            vkBinding.descriptorType = toVkDescriptorType(binding.descriptorType);
            vkBinding.descriptorCount = binding.descriptorCount;
            vkBinding.stageFlags = toVkShaderStageFlags(binding.stageFlags);
            vkBinding.pImmutableSamplers = nullptr;

            vkBindings.push_back(vkBinding);
        }

        VkDescriptorSetLayoutCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.bindingCount = static_cast<uint32_t>(vkBindings.size());
        createInfo.pBindings = vkBindings.data();

        if (vkCreateDescriptorSetLayout(m_device.native(), &createInfo, nullptr, &m_layout) != VK_SUCCESS) {
            throw std::runtime_error("[LavaVK ERROR] Failed to create descriptor set layout!");
        }
    }

    DescriptorSetLayout::~DescriptorSetLayout() {
        if (m_layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(m_device.native(), m_layout, nullptr);
        }
    }


    DescriptorPool::Builder &DescriptorPool::Builder::addPoolSize(DescriptorType descriptorType, uint32_t count) {
        m_poolCounts[descriptorType] += count;
        return *this;
    }

    DescriptorPool::Builder &DescriptorPool::Builder::setPoolFlags(VkDescriptorPoolCreateFlags flags) {
        m_poolFlags = flags;
        return *this;
    }

    DescriptorPool::Builder &DescriptorPool::Builder::setMaxSets(uint32_t count) {
        m_maxSets = count;
        return *this;
    }

    std::unique_ptr<DescriptorPool> DescriptorPool::Builder::build() const {
        std::vector<VkDescriptorPoolSize> poolSizes;
        poolSizes.reserve(m_poolCounts.size());

        for (const auto &[type, count]: m_poolCounts) {
            poolSizes.push_back({toVkDescriptorType(type), count});
        }

        return std::make_unique<DescriptorPool>(m_device, m_maxSets, m_poolFlags, poolSizes);
    }

    DescriptorPool::DescriptorPool(
        Device &device,
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags flags,
        const std::vector<VkDescriptorPoolSize> &poolSizes)
        : m_device(device) {
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = maxSets;
        poolInfo.flags = flags;

        if (vkCreateDescriptorPool(m_device.native(), &poolInfo, nullptr, &m_pool) != VK_SUCCESS) {
            throw std::runtime_error("[LavaVK ERROR] Failed to create descriptor pool!");
        }
    }

    DescriptorPool::~DescriptorPool() {
        if (m_pool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_device.native(), m_pool, nullptr);
        }
    }

    bool DescriptorPool::allocateDescriptorSet(VkDescriptorSetLayout layout, VkDescriptorSet &descriptorSet) const {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_pool;
        allocInfo.pSetLayouts = &layout;
        allocInfo.descriptorSetCount = 1;

        return vkAllocateDescriptorSets(m_device.native(), &allocInfo, &descriptorSet) == VK_SUCCESS;
    }

    void DescriptorPool::freeDescriptorSets(const std::vector<VkDescriptorSet> &descriptorSets) const {
        vkFreeDescriptorSets(m_device.native(), m_pool, static_cast<uint32_t>(descriptorSets.size()),
                             descriptorSets.data());
    }


    DescriptorWriter::DescriptorWriter(DescriptorSetLayout &setLayout, DescriptorPool &pool)
        : m_setLayout(setLayout), m_pool(pool) {
    }

    DescriptorWriter &DescriptorWriter::writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo) {
        const auto &bindings = m_setLayout.getBindings();
        auto bindingDescription = bindings.at(binding);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = toVkDescriptorType(bindingDescription.descriptorType);
        write.dstBinding = binding;
        write.pBufferInfo = bufferInfo;
        write.descriptorCount = bindingDescription.descriptorCount;

        m_writes.push_back(write);
        return *this;
    }

    DescriptorWriter &DescriptorWriter::writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo) {
        const auto &bindings = m_setLayout.getBindings();
        auto bindingDescription = bindings.at(binding);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = toVkDescriptorType(bindingDescription.descriptorType);
        write.dstBinding = binding;
        write.pImageInfo = imageInfo;
        write.descriptorCount = bindingDescription.descriptorCount;

        m_writes.push_back(write);
        return *this;
    }

    bool DescriptorWriter::build(VkDescriptorSet &set) {
        bool success = m_pool.allocateDescriptorSet(m_setLayout.native(), set);
        if (!success) {
            return false;
        }
        overwrite(set);
        return true;
    }

    void DescriptorWriter::overwrite(VkDescriptorSet &set) {
        for (auto &write: m_writes) {
            write.dstSet = set;
        }
        vkUpdateDescriptorSets(m_pool.m_device.native(), static_cast<uint32_t>(m_writes.size()), m_writes.data(), 0,
                               nullptr);
    }

    GraphicsPipeline::GraphicsPipeline(
        Device &device,
        const GraphicsPipelineCreateInfo &info)
        : m_device(device) {
        if (!info.vertexShader)
            throw std::runtime_error("[LavaVK ERROR] Vertex shader is null.");

        if (!info.fragmentShader)
            throw std::runtime_error("[LavaVK ERROR] Fragment shader is null.");

        if (!info.layout)
            throw std::runtime_error("[LavaVK ERROR] PipelineLayout is null.");

        if (!info.renderPass)
            throw std::runtime_error("[LavaVK ERROR] RenderPass is null.");

        VkPipelineShaderStageCreateInfo shaderStages[2]{};

        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = info.vertexShader->native();
        shaderStages[0].pName = "main";

        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = info.fragmentShader->native();
        shaderStages[1].pName = "main";

        VkPipelineVertexInputStateCreateInfo vertexInput{};

        vertexInput.sType =
                VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        vertexInput.vertexBindingDescriptionCount = 0;
        vertexInput.vertexAttributeDescriptionCount = 0;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};

        inputAssembly.sType =
                VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

        inputAssembly.topology =
                static_cast<VkPrimitiveTopology>(info.topology);

        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};

        viewportState.sType =
                VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};

        rasterizer.sType =
                VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;

        rasterizer.polygonMode =
                static_cast<VkPolygonMode>(info.polygonMode);

        rasterizer.lineWidth = 1.0f;

        rasterizer.cullMode =
                static_cast<VkCullModeFlags>(info.cullMode);

        rasterizer.frontFace =
                static_cast<VkFrontFace>(info.frontFace);

        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};

        multisampling.sType =
                VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

        multisampling.rasterizationSamples = info.samples;
        multisampling.sampleShadingEnable = VK_FALSE;
        VkPipelineDepthStencilStateCreateInfo depthStencil{};

        depthStencil.sType =
                VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        depthStencil.depthTestEnable =
                info.depthTest ? VK_TRUE : VK_FALSE;

        depthStencil.depthWriteEnable =
                info.depthWrite ? VK_TRUE : VK_FALSE;

        depthStencil.depthCompareOp =
                static_cast<VkCompareOp>(info.depthCompare);

        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};

        colorBlendAttachment.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;

        colorBlendAttachment.blendEnable =
                info.blending ? VK_TRUE : VK_FALSE;

        colorBlendAttachment.srcColorBlendFactor =
                static_cast<VkBlendFactor>(info.srcColorBlendFactor);

        colorBlendAttachment.dstColorBlendFactor =
                static_cast<VkBlendFactor>(info.dstColorBlendFactor);

        colorBlendAttachment.colorBlendOp =
                static_cast<VkBlendOp>(info.colorBlendOperation);

        colorBlendAttachment.srcAlphaBlendFactor =
                static_cast<VkBlendFactor>(info.srcAlphaBlendFactor);

        colorBlendAttachment.dstAlphaBlendFactor =
                static_cast<VkBlendFactor>(info.dstAlphaBlendFactor);

        colorBlendAttachment.alphaBlendOp =
                static_cast<VkBlendOp>(info.alphaBlendOperation);

        VkPipelineColorBlendStateCreateInfo colorBlending{};

        colorBlending.sType =
                VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

        colorBlending.logicOpEnable = VK_FALSE;

        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        std::vector<VkDynamicState> dynamicStates;

        if (info.dynamicViewport)
            dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);

        if (info.dynamicScissor)
            dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);

        VkPipelineDynamicStateCreateInfo dynamicState{};

        dynamicState.sType =
                VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

        dynamicState.dynamicStateCount =
                static_cast<uint32_t>(dynamicStates.size());

        dynamicState.pDynamicStates =
                dynamicStates.data();

        VkGraphicsPipelineCreateInfo pipelineInfo{};

        pipelineInfo.sType =
                VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;

        pipelineInfo.pVertexInputState = &vertexInput;
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
            throw std::runtime_error(
                "[LavaVK ERROR] Failed to create graphics pipeline.");
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

    GraphicsPipeline &
    GraphicsPipeline::operator=(GraphicsPipeline &&other) noexcept {
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
