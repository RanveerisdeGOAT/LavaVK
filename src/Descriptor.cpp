#include "LavaVK/Descriptor.hpp"

#include <vector>

#include "LavaVK/LavaVK.hpp"
#include "LavaVK/Shader.hpp"
#include "LavaVK/Device.hpp"

namespace LavaVK {
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
            LAVAVK_ERROR("[LavaVK ERROR] Failed to create descriptor set layout!");
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

    // --- DescriptorPool ---

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
            LAVAVK_ERROR("[LavaVK ERROR] Failed to create descriptor pool!");
        }
    }

    DescriptorPool::~DescriptorPool() {
        if (m_pool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(m_device.native(), m_pool, nullptr);
        }
    }

    DescriptorSet DescriptorPool::allocateDescriptorSet(const DescriptorSetLayout &layout) const {
        DescriptorSet descriptorSet = VK_NULL_HANDLE;
        VkDescriptorSetLayout vkLayout = layout.native();

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_pool;
        allocInfo.pSetLayouts = &vkLayout;
        allocInfo.descriptorSetCount = 1;

        if (vkAllocateDescriptorSets(m_device.native(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
            LAVAVK_ERROR("Failed to allocate descriptor set from pool!");
        }

        return descriptorSet;
    }

    void DescriptorPool::freeDescriptorSets(const std::vector<VkDescriptorSet> &descriptorSets) const {
        vkFreeDescriptorSets(m_device.native(), m_pool, static_cast<uint32_t>(descriptorSets.size()),
                             descriptorSets.data());
    }

    // --- DescriptorPool::Writer ---

    DescriptorPool::Writer::Writer(DescriptorPool &pool, DescriptorSetLayout &setLayout)
        : m_pool(pool), m_setLayout(setLayout) {
    }

    DescriptorPool::Writer &DescriptorPool::Writer::writeBuffer(uint32_t binding, const DescriptorBuffer *bufferInfo) {
        const auto &bindings = m_setLayout.getBindings();
        auto bindingDescription = bindings.at(binding);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = toVkDescriptorType(bindingDescription.descriptorType);
        write.dstBinding = binding;
        write.pBufferInfo = &bufferInfo->native();
        write.descriptorCount = bindingDescription.descriptorCount;

        m_writes.push_back(write);
        return *this;
    }

    DescriptorPool::Writer &DescriptorPool::Writer::writeImage(uint32_t binding, const DescriptorImage *imageInfo) {
        const auto &bindings = m_setLayout.getBindings();
        auto bindingDescription = bindings.at(binding);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = toVkDescriptorType(bindingDescription.descriptorType);
        write.dstBinding = binding;
        write.pImageInfo = &imageInfo->native();
        write.descriptorCount = bindingDescription.descriptorCount;

        m_writes.push_back(write);
        return *this;
    }

    DescriptorSet DescriptorPool::Writer::build() {
        DescriptorSet set = m_pool.allocateDescriptorSet(m_setLayout);

        if (set == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to allocate descriptor set: DescriptorPool exhausted!");
        }

        overwrite(set);
        return set;
    }

    void DescriptorPool::Writer::overwrite(DescriptorSet set) {
        if (set == VK_NULL_HANDLE) {
            LAVAVK_ERROR("[LavaVK ERROR] Cannot overwrite a null DescriptorSet handle!");
            return;
        }

        if (m_writes.empty()) {
            return; // Nothing to update
        }

        for (auto &write: m_writes) {
            write.dstSet = set;
        }

        vkUpdateDescriptorSets(
            m_pool.device().native(),
            static_cast<uint32_t>(m_writes.size()),
            m_writes.data(),
            0,
            nullptr
        );
    }
}
