#include "LavaVK/SwapChain.hpp"

#include "LavaVK/Queue.hpp"
#include "LavaVK/Texture.hpp"


namespace LavaVK {
    namespace {
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(
            const std::vector<VkSurfaceFormatKHR> &availableFormats,
            VkFormat preferredFormat) {
            for (const auto &availableFormat: availableFormats) {
                if (availableFormat.format == preferredFormat &&
                    availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    return availableFormat;
                }
            }
            return availableFormats[0];
        }

        VkPresentModeKHR chooseSwapPresentMode(
            const std::vector<VkPresentModeKHR> &availablePresentModes) {
            for (const auto &availablePresentMode: availablePresentModes) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    return availablePresentMode;
                }
            }
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        VkExtent2D chooseSwapExtent(
            const VkSurfaceCapabilitiesKHR &capabilities,
            VkExtent2D preferredExtent) {
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
                return capabilities.currentExtent;
            }

            VkExtent2D actualExtent = preferredExtent;
            actualExtent.width = std::clamp(
                actualExtent.width,
                capabilities.minImageExtent.width,
                capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(
                actualExtent.height,
                capabilities.minImageExtent.height,
                capabilities.maxImageExtent.height);

            return actualExtent;
        }
    } // namespace

    SwapChain::SwapChain(
        Device &device,
        Surface &surface,
        RenderPass *renderPass,
        VkFormat colorFormat,
        VkFormat depthFormat,
        VkExtent2D extent)
        : m_device(device),
          m_surface(surface),
          m_renderPass(renderPass),
          m_colorFormat(colorFormat),
          m_depthFormat(depthFormat),
          m_extent(extent) {
        // Create synchronization primitives (created once and kept alive during recreations)
        m_imageAvailable.reserve(MAX_FRAMES_IN_FLIGHT);
        m_inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_imageAvailable.emplace_back(m_device);
            m_inFlightFences.emplace_back(m_device, true); // Create signaled
        }

        createPassed();
    }

    SwapChain::SwapChain(
        Device &device,
        Surface &surface,
        VkFormat colorFormat,
        VkFormat depthFormat,
        VkExtent2D extent)
        : m_device(device),
          m_surface(surface),
          m_colorFormat(colorFormat),
          m_depthFormat(depthFormat),
          m_extent(extent) {
        m_imageAvailable.reserve(MAX_FRAMES_IN_FLIGHT);
        m_inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_imageAvailable.emplace_back(m_device);
            m_inFlightFences.emplace_back(m_device, true);
        }

        createDynamic();
    }

    SwapChain::~SwapChain() {
        cleanup();
    }

    void SwapChain::createPassed() {
        VkPhysicalDevice physicalDevice = m_device.physical();
        VkSurfaceKHR surfaceHandle = m_surface.native();

        // 1. Query Capabilities
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surfaceHandle, &capabilities);

        // 2. Query Formats
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surfaceHandle, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surfaceHandle, &formatCount, formats.data());

        // 3. Query Present Modes
        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surfaceHandle, &presentModeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surfaceHandle, &presentModeCount,
                                                  presentModes.data());

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats, m_colorFormat);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes);
        VkExtent2D swapExtent = chooseSwapExtent(capabilities, m_extent);

        m_format = surfaceFormat.format;
        m_extent = swapExtent;

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surfaceHandle;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = swapExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        uint32_t graphicsQueueIndex = m_device.getQueueFamily(QueueType::GRAPHICS);
        uint32_t presentQueueIndex = m_device.getQueueFamily(QueueType::PRESENT);
        uint32_t queueFamilyIndices[] = {graphicsQueueIndex, presentQueueIndex};

        if (graphicsQueueIndex != presentQueueIndex) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        if (vkCreateSwapchainKHR(m_device.native(), &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
            LAVAVK_ERROR("[LavaVK ERROR] Failed to create Vulkan swapchain.");
        }

        // 4. Retrieve raw VkImages & encapsulate them inside LavaVK::Image wrappers
        uint32_t rawImageCount = 0;
        vkGetSwapchainImagesKHR(m_device.native(), m_swapchain, &rawImageCount, nullptr);
        std::vector<VkImage> rawImages(rawImageCount);
        vkGetSwapchainImagesKHR(m_device.native(), m_swapchain, &rawImageCount, rawImages.data());

        VkExtent3D imageExtent{m_extent.width, m_extent.height, 1};
        m_images.reserve(rawImageCount);
        for (VkImage rawImg: rawImages) {
            m_images.emplace_back(m_device, rawImg, m_format, VK_IMAGE_ASPECT_COLOR_BIT, imageExtent);
        }

        // 5. Create Per-Swapchain-Image RenderFinished Semaphores
        m_renderFinished.clear();
        m_renderFinished.reserve(rawImageCount);
        for (size_t i = 0; i < rawImageCount; i++) {
            m_renderFinished.emplace_back(m_device);
        }

        m_imagesInFlight.assign(rawImageCount, VK_NULL_HANDLE);

        // 6. Create Depth Image Attachment
        m_depthImage = std::make_unique<Image>(
            m_device,
            ImageCreateInfo{
                .type = ImageType::IMAGE_2D,
                .extent = {m_extent.width, m_extent.height, 1},
                .format = m_depthFormat,
                .usage = ImageUsage::DEPTH_ATTACHMENT,
                .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .memory = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            }
        );

        // 7. Create Framebuffers
        m_framebuffers.reserve(m_images.size());
        for (size_t i = 0; i < m_images.size(); i++) {
            std::vector<Image *> attachments = {&m_images[i], m_depthImage.get()};
            m_framebuffers.emplace_back(m_device, *m_renderPass, attachments);
        }
    }

    void SwapChain::createDynamic() {
        VkPhysicalDevice physicalDevice = m_device.physical();
        VkSurfaceKHR surfaceHandle = m_surface.native();

        VkSurfaceCapabilitiesKHR capabilities{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            physicalDevice,
            surfaceHandle,
            &capabilities
        );

        uint32_t formatCount = 0;

        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice,
            surfaceHandle,
            &formatCount,
            nullptr
        );

        std::vector<VkSurfaceFormatKHR> formats(formatCount);

        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice,
            surfaceHandle,
            &formatCount,
            formats.data()
        );

        uint32_t presentModeCount = 0;

        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice,
            surfaceHandle,
            &presentModeCount,
            nullptr
        );

        std::vector<VkPresentModeKHR> presentModes(presentModeCount);

        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice,
            surfaceHandle,
            &presentModeCount,
            presentModes.data()
        );


        VkSurfaceFormatKHR surfaceFormat =
                chooseSwapSurfaceFormat(formats, m_colorFormat);

        VkPresentModeKHR presentMode =
                chooseSwapPresentMode(presentModes);

        VkExtent2D swapExtent =
                chooseSwapExtent(capabilities, m_extent);


        m_format = surfaceFormat.format;
        m_extent = swapExtent;

        uint32_t imageCount = capabilities.minImageCount + 1;

        if (capabilities.maxImageCount > 0 &&
            imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }


        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType =
                VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

        createInfo.surface = surfaceHandle;

        createInfo.minImageCount = imageCount;

        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;

        createInfo.imageExtent = swapExtent;

        createInfo.imageArrayLayers = 1;


        // Dynamic rendering uses color attachments directly
        createInfo.imageUsage =
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


        uint32_t graphicsQueueIndex =
                m_device.getQueueFamily(QueueType::GRAPHICS);

        uint32_t presentQueueIndex =
                m_device.getQueueFamily(QueueType::PRESENT);


        uint32_t queueFamilyIndices[] =
        {
            graphicsQueueIndex,
            presentQueueIndex
        };


        if (graphicsQueueIndex != presentQueueIndex) {
            createInfo.imageSharingMode =
                    VK_SHARING_MODE_CONCURRENT;

            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode =
                    VK_SHARING_MODE_EXCLUSIVE;
        }


        createInfo.preTransform =
                capabilities.currentTransform;

        createInfo.compositeAlpha =
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        createInfo.presentMode =
                presentMode;

        createInfo.clipped = VK_TRUE;


        if (vkCreateSwapchainKHR(
                m_device.native(),
                &createInfo,
                nullptr,
                &m_swapchain) != VK_SUCCESS) {
            LAVAVK_ERROR(
                "[LavaVK ERROR] Failed to create Vulkan swapchain."
            );
        }

        uint32_t rawImageCount = 0;

        vkGetSwapchainImagesKHR(
            m_device.native(),
            m_swapchain,
            &rawImageCount,
            nullptr
        );


        std::vector<VkImage> rawImages(rawImageCount);


        vkGetSwapchainImagesKHR(
            m_device.native(),
            m_swapchain,
            &rawImageCount,
            rawImages.data()
        );


        m_images.clear();
        m_images.reserve(rawImageCount);


        VkExtent3D imageExtent{
            m_extent.width,
            m_extent.height,
            1
        };


        for (VkImage image: rawImages) {
            m_images.emplace_back(
                m_device,
                image,
                m_format,
                VK_IMAGE_ASPECT_COLOR_BIT,
                imageExtent
            );
        }

        m_renderFinished.clear();
        m_renderFinished.reserve(rawImageCount);

        for (size_t i = 0; i < rawImageCount; i++) {
            m_renderFinished.emplace_back(m_device);
        }


        m_imagesInFlight.assign(
            rawImageCount,
            VK_NULL_HANDLE
        );

        m_depthImage = std::make_unique<Image>(
            m_device,
            ImageCreateInfo{
                .type = ImageType::IMAGE_2D,

                .extent = {
                    m_extent.width,
                    m_extent.height,
                    1
                },

                .format = m_depthFormat,

                .usage = ImageUsage::DEPTH_ATTACHMENT,

                .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,

                .samples = VK_SAMPLE_COUNT_1_BIT,

                .tiling = VK_IMAGE_TILING_OPTIMAL,

                .memory =
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            }
        );
    }

    void SwapChain::cleanup() {
        // 1. Destroy Framebuffers first (depend on Images & RenderPass)
        m_framebuffers.clear();

        // 2. Destroy Depth Image Attachment
        m_depthImage.reset();

        // 3. Destroy Color Image wrappers
        m_images.clear();

        // 4. Destroy native VkSwapchainKHR handle
        if (m_swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_device.native(), m_swapchain, nullptr);
            m_swapchain = VK_NULL_HANDLE;
        }
    }

    void SwapChain::recreate(bool passed) {
        VkExtent2D newExtent = getLatestExtent();

        // Handle Minimization: If width or height is 0, pause execution until un-minimized
        while (newExtent.width == 0 || newExtent.height == 0) {
            newExtent = getLatestExtent();
        }

        // Wait for GPU work to complete before tearing down existing resources
        m_device.waitIdle();

        // Clean up old swapchain resources
        cleanup();

        m_extent = newExtent;

        // Re-create swapchain resources with updated extent
        if (passed) createPassed();
        else createDynamic();
    }

    VkExtent2D SwapChain::getLatestExtent() const {
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            m_device.physical(),
            m_surface.native(),
            &capabilities
        );

        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        }

        return m_extent;
    }

    Result SwapChain::acquireImage(uint32_t &imageIndex) {
        m_inFlightFences[m_currentFrame].wait(UINT64_MAX);

        VkResult result = vkAcquireNextImageKHR(
            m_device.native(),
            m_swapchain,
            std::numeric_limits<uint64_t>::max(),
            m_imageAvailable[m_currentFrame].native(),
            VK_NULL_HANDLE,
            &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            return result;
        }

        if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(m_device.native(), 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }

        m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrame].native();
        m_inFlightFences[m_currentFrame].reset();

        return result;
    }

    Result SwapChain::present(uint32_t imageIndex) {
        VkSemaphore waitSemaphores[] = {m_renderFinished[imageIndex].native()};

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = waitSemaphores;

        VkSwapchainKHR swapchains[] = {m_swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;

        VkResult result = vkQueuePresentKHR(m_device.getQueue(QueueType::PRESENT).native(), &presentInfo);

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        return result;
    }

    SwapChain::SwapChain(SwapChain &&other) noexcept
        : m_device(other.m_device),
          m_surface(other.m_surface),
          m_renderPass(other.m_renderPass),
          m_swapchain(other.m_swapchain),
          m_colorFormat(other.m_colorFormat),
          m_depthFormat(other.m_depthFormat),
          m_format(other.m_format),
          m_extent(other.m_extent),
          m_images(std::move(other.m_images)),
          m_depthImage(std::move(other.m_depthImage)),
          m_framebuffers(std::move(other.m_framebuffers)),
          m_currentFrame(other.m_currentFrame),
          m_imageAvailable(std::move(other.m_imageAvailable)),
          m_inFlightFences(std::move(other.m_inFlightFences)),
          m_renderFinished(std::move(other.m_renderFinished)),
          m_imagesInFlight(std::move(other.m_imagesInFlight)) {
        other.m_swapchain = VK_NULL_HANDLE;
    }

    SwapChain &SwapChain::operator=(SwapChain &&other) noexcept {
        if (this != &other) {
            cleanup();

            m_swapchain = other.m_swapchain;
            m_colorFormat = other.m_colorFormat;
            m_depthFormat = other.m_depthFormat;
            m_format = other.m_format;
            m_extent = other.m_extent;
            m_images = std::move(other.m_images);
            m_depthImage = std::move(other.m_depthImage);
            m_framebuffers = std::move(other.m_framebuffers);
            m_currentFrame = other.m_currentFrame;
            m_imageAvailable = std::move(other.m_imageAvailable);
            m_inFlightFences = std::move(other.m_inFlightFences);
            m_renderFinished = std::move(other.m_renderFinished);
            m_imagesInFlight = std::move(other.m_imagesInFlight);

            other.m_swapchain = VK_NULL_HANDLE;
        }

        return *this;
    }
} // namespace LavaVK
