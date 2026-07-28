#include "LavaVK/SwapChain.hpp"
#include "LavaVK/Device.hpp"
#include "LavaVK/Surface.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

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
        VkFormat format,
        VkExtent2D extent)
        : m_device(device),
          m_surface(surface) {
        VkPhysicalDevice physicalDevice = m_device.physical();
        VkSurfaceKHR surfaceHandle = m_surface.native();

        // Query Capabilities
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surfaceHandle, &capabilities);

        // Query Formats
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surfaceHandle, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surfaceHandle, &formatCount, formats.data());

        // Query Present Modes
        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surfaceHandle, &presentModeCount, nullptr);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surfaceHandle, &presentModeCount,
                                                  presentModes.data());

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats, format);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes);
        VkExtent2D swapExtent = chooseSwapExtent(capabilities, extent);

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
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

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
            throw std::runtime_error("[LavaVK ERROR] Failed to create Vulkan swapchain.");
        }

        // Retrieve raw VkImages
        uint32_t rawImageCount = 0;
        vkGetSwapchainImagesKHR(m_device.native(), m_swapchain, &rawImageCount, nullptr);
        std::vector<VkImage> rawImages(rawImageCount);
        vkGetSwapchainImagesKHR(m_device.native(), m_swapchain, &rawImageCount, rawImages.data());

        VkExtent3D imageExtent{m_extent.width, m_extent.height, 1};
        m_images.reserve(rawImageCount);
        for (VkImage rawImg: rawImages) {
            m_images.emplace_back(m_device, rawImg, m_format, VK_IMAGE_ASPECT_COLOR_BIT, imageExtent);
        }

        // Create Per-Frame-In-Flight Synchronization Objects
        m_imageAvailable.reserve(MAX_FRAMES_IN_FLIGHT);
        m_inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_imageAvailable.emplace_back(m_device);
            m_inFlightFences.emplace_back(m_device, true); // Create signaled
        }

        // Create Per-Swapchain-Image RenderFinished Semaphores
        m_renderFinished.reserve(rawImageCount);
        for (size_t i = 0; i < rawImageCount; i++) {
            m_renderFinished.emplace_back(m_device);
        }

        m_imagesInFlight.resize(rawImageCount, VK_NULL_HANDLE);
    }

    SwapChain::~SwapChain() {
        if (m_swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_device.native(), m_swapchain, nullptr);
        }
    }

    SwapChain::SwapChain(SwapChain &&other) noexcept
        : m_device(other.m_device),
          m_surface(other.m_surface),
          m_swapchain(other.m_swapchain),
          m_format(other.m_format),
          m_extent(other.m_extent),
          m_images(std::move(other.m_images)),
          m_currentFrame(other.m_currentFrame),
          m_imageAvailable(std::move(other.m_imageAvailable)),
          m_inFlightFences(std::move(other.m_inFlightFences)),
          m_renderFinished(std::move(other.m_renderFinished)),
          m_imagesInFlight(std::move(other.m_imagesInFlight)) {
        other.m_swapchain = VK_NULL_HANDLE;
    }

    SwapChain &SwapChain::operator=(SwapChain &&other) noexcept {
        if (this != &other) {
            m_images.clear();

            if (m_swapchain != VK_NULL_HANDLE) {
                vkDestroySwapchainKHR(m_device.native(), m_swapchain, nullptr);
            }

            m_surface = std::move(other.m_surface);
            m_swapchain = other.m_swapchain;
            m_format = other.m_format;
            m_extent = other.m_extent;
            m_images = std::move(other.m_images);
            m_currentFrame = other.m_currentFrame;
            m_imageAvailable = std::move(other.m_imageAvailable);
            m_inFlightFences = std::move(other.m_inFlightFences);
            m_renderFinished = std::move(other.m_renderFinished);
            m_imagesInFlight = std::move(other.m_imagesInFlight);

            other.m_swapchain = VK_NULL_HANDLE;
        }

        return *this;
    }

    VkResult SwapChain::acquireImage(uint32_t &imageIndex) {
        // 1. Wait for GPU frame completion
        m_inFlightFences[m_currentFrame].wait(UINT64_MAX);

        // 2. Acquire image
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

        // 3. Ensure swapchain image isn't currently in use by an older frame in flight
        if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(m_device.native(), 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }

        m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrame].native();
        m_inFlightFences[m_currentFrame].reset();

        return result;
    }

    VkResult SwapChain::present(uint32_t imageIndex) {
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

        // Advance frame slot
        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        return result;
    }
} // namespace LavaVK
