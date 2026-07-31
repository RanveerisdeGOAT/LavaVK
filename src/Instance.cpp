#include "LavaVK/Instance.hpp"
#include "LavaVK/Error.hpp"

namespace LavaVK {
    Instance::Instance(const InstanceCreateInfo &info) {
        VkApplicationInfo app{};
        app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app.pApplicationName = info.applicationName.c_str();
        app.applicationVersion = info.applicationVersion;
        app.pEngineName = "LavaVK";
        app.engineVersion = VK_MAKE_VERSION(0, 10, 0);
        app.apiVersion = VK_API_VERSION_1_3;

        std::vector<const char *> extensions = info.extensions;

#ifndef NDEBUG
        if (info.enableValidation) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        VkInstanceCreateInfo create{};
        create.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create.pApplicationInfo = &app;

        create.enabledExtensionCount = static_cast<uint32_t>(extensions.size());

        create.ppEnabledExtensionNames = extensions.data();

#ifndef NDEBUG
        const char *layers[]{"VK_LAYER_KHRONOS_validation"};

        if (info.enableValidation) {
            create.enabledLayerCount = 1;
            create.ppEnabledLayerNames = layers;
        }
#endif

        if (vkCreateInstance(&create, nullptr, &m_instance) != VK_SUCCESS)
            LAVAVK_ERROR("[LavaVK ERROR] Failed to create Vulkan instance.");
    }

    Instance::~Instance() {
        if (m_instance)
            vkDestroyInstance(m_instance, nullptr);
    }

    Instance::Instance(Instance &&other) noexcept {
        m_instance = other.m_instance;
        other.m_instance = VK_NULL_HANDLE;
    }

    Instance &Instance::operator=(Instance &&other) noexcept {
        if (this != &other) {
            if (m_instance)
                vkDestroyInstance(m_instance, nullptr);

            m_instance = other.m_instance;
            other.m_instance = VK_NULL_HANDLE;
        }

        return *this;
    }
}
