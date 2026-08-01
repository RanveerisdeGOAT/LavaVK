#ifndef LAVAVK_SWAPCHAIN_HPP
#define LAVAVK_SWAPCHAIN_HPP

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include <algorithm>
#include <limits>
#include <stdexcept>

#include "Buffer.hpp"
#include "Surface.hpp"
#include "Sync.hpp"
#include "Device.hpp"
#include "Surface.hpp"
#include "Texture.hpp"

namespace LavaVK {
    class RenderPass;
    /**
     * @brief Maximum number of frames that can be processed concurrently by the CPU and GPU.
     * Double-buffering (2) or triple-buffering allows the CPU to record commands for frame N+1
     * while the GPU renders frame N.
     */
    constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    /**
     * @brief Manages the Vulkan Swapchain and associated frame/image synchronization primitives.
     * The SwapChain class abstracts creation of the VkSwapchainKHR handle, acquisition of
     * swapchain images, queue presentation, and synchronization objects (semaphores and fences)
     * required for multi-frame in-flight rendering.
     */
    class SwapChain {
    public:
        /**
         * @brief Constructs a LavaVK SwapChain instance and creates internal synchronization objects.
         * Queries physical device capabilities, chooses optimal surface formats, present modes,
         * and extents, constructs the VkSwapchainKHR, and creates per-frame and per-image sync objects.
         * @param device Logical device wrapper used to manage Vulkan resources.
         * @param surface Presentation surface to bind the swapchain against.
         * @param renderPass Render pass compatible with framebuffers created by this swapchain.
         * @param colorFormat Preferred color attachment format.
         * @param depthFormat Preferred depth attachment format.
         * @param extent Desired width and height of the swapchain images in pixels.
         */
        SwapChain(
            Device &device,
            Surface &surface,
            RenderPass &renderPass,
            VkFormat colorFormat,
            VkFormat depthFormat,
            VkExtent2D extent);

        /**
         * @brief Destroys the swapchain object and releases the native VkSwapchainKHR handle.
         */
        ~SwapChain();

        // Disable copy semantics as swapchain and sync primitives own unique Vulkan handles
        SwapChain(const SwapChain &) = delete;

        SwapChain &operator=(const SwapChain &) = delete;

        /**
         * @brief Move constructor.
         * Transfers ownership of Vulkan swapchain handles and synchronization vectors.
         */
        SwapChain(SwapChain &&other) noexcept;

        /**
         * @brief Move assignment operator.
         * Cleans up existing swapchain resources and transfers ownership from @p other.
         */
        SwapChain &operator=(SwapChain &&other) noexcept;

        /**
         * @brief Prepares for rendering the next frame by waiting on fences and acquiring an image index.
         * 1. Waits for the CPU-GPU fence of the current frame slot (`m_currentFrame`).
         * 2. Acquires the next available image index from the Vulkan presentation engine.
         * 3. Ensures the acquired swapchain image is not still in flight from an earlier frame.
         * 4. Resets the current frame's in-flight fence.
         * @param[out] imageIndex Variable populated with the index of the acquired swapchain image.
         * @return Result Status code.
         */
        Result acquireImage(uint32_t &imageIndex);

        /**
         * @brief Submits the acquired swapchain image to the presentation engine and advances the frame slot.
         * Waits on the `renderFinished` semaphore corresponding to @p imageIndex, submits the presentation
         * request to the queue, and advances `m_currentFrame` modulo `MAX_FRAMES_IN_FLIGHT`.
         * @param imageIndex The index of the swapchain image to present.
         * @return Result Status code.
         */
        Result present(uint32_t imageIndex);

        /**
         * @brief Gets the total number of images retrieved from the Vulkan swapchain.
         * @return Image count (typically 2 or 3).
         */
        [[nodiscard]] uint32_t imageCount() const {
            return static_cast<uint32_t>(m_images.size());
        }

        /**
         * @brief Retrieves a const reference to a wrapped swapchain image by index.
         * @param index Swapchain image index.
         * @return Const reference to LavaVK::Image.
         */
        [[nodiscard]] const Image &image(uint32_t index) const {
            return m_images[index];
        }

        /**
         * @brief Retrieves a mutable reference to a wrapped swapchain image by index.
         * @param index Swapchain image index.
         * @return Reference to LavaVK::Image.
         */
        [[nodiscard]] Image &image(uint32_t index) {
            return m_images[index];
        }

        /**
         * @brief Retrieves all wrapped swapchain images.
         * @return Const reference to vector of LavaVK::Image objects.
         */
        [[nodiscard]] const std::vector<Image> &images() const {
            return m_images;
        }

        /**
         * @brief Gets the 2D resolution extent of the swapchain images.
         * @return VkExtent2D containing width and height.
         */
        [[nodiscard]] VkExtent2D extent() const {
            return m_extent;
        }

        /**
         * @brief Gets the Vulkan color format selected for the swapchain.
         * @return VkFormat enum value.
         */
        [[nodiscard]] VkFormat format() const {
            return m_format;
        }

        /**
         * @brief Returns the native Vulkan VkSwapchainKHR handle.
         * @return Raw VkSwapchainKHR handle.
         */
        [[nodiscard]] VkSwapchainKHR native() const {
            return m_swapchain;
        }

        /**
         * @brief Gets the surface object associated with this swapchain.
         * @return Const reference to LavaVK::Surface.
         */
        [[nodiscard]] const Surface &surface() const {
            return m_surface;
        }

        /**
         * @brief Returns the index of the current frame in flight [0..MAX_FRAMES_IN_FLIGHT - 1].
         * @return Current frame slot index.
         */
        [[nodiscard]] size_t currentFrame() const {
            return m_currentFrame;
        }

        // --- Synchronization Accessors ---

        /**
         * @brief Returns the GPU-GPU semaphore signaled when the current frame slot's image is acquired.
         * Pass this to `VkSubmitInfo::pWaitSemaphores` when submitting command buffers for the current frame.
         * @return Const reference to the current frame's image-available semaphore.
         */
        [[nodiscard]] const Semaphore &imageAvailableSemaphore() const {
            return m_imageAvailable[m_currentFrame];
        }

        /**
         * @brief Returns the GPU-GPU semaphore signaled when rendering to a specific swapchain image is completed.
         * Pass this to `VkSubmitInfo::pSignalSemaphores` when submitting draw commands for @p imageIndex.
         * @param imageIndex Swapchain image index being rendered to.
         * @return Const reference to the render-finished semaphore bound to @p imageIndex.
         */
        [[nodiscard]] const Semaphore &renderFinishedSemaphore(uint32_t imageIndex) const {
            return m_renderFinished[imageIndex];
        }

        /**
         * @brief Returns the CPU-GPU fence used to synchronize CPU command buffer recording with GPU execution.
         * Pass this to `vkQueueSubmit` to be signaled when GPU work for the current frame finishes.
         * @return Const reference to the current frame's in-flight fence.
         */
        [[nodiscard]] const Fence &inFlightFence() const {
            return m_inFlightFences[m_currentFrame];
        }

        /**
         * @brief Returns reference to Framebuffer bound to swapchain image index.
         */
        [[nodiscard]] const Framebuffer &framebuffer(uint32_t index) const {
            return m_framebuffers[index];
        }

        /**
         * @brief Recreates the swapchain when the window size or surface capabilities change.
         */
        void recreate();

        /**
         * @brief Queries the latest surface extent directly from Vulkan.
         */
        [[nodiscard]] VkExtent2D getLatestExtent() const;

    private:
        /**
         * @brief Allocates and initializes native VkSwapchainKHR, images, views, depth attachment, and framebuffers.
         */
        void create();

        /**
         * @brief Destroys framebuffers, depth attachment, image views, and native swapchain handles.
         */
        void cleanup();

        Device &m_device; ///< Reference to logical device wrapper.
        Surface &m_surface; ///< Reference to presentation surface wrapper.
        RenderPass &m_renderPass; ///< Reference to render pass used to build framebuffers.

        VkSwapchainKHR m_swapchain = VK_NULL_HANDLE; ///< Native Vulkan swapchain handle.

        VkFormat m_colorFormat = VK_FORMAT_UNDEFINED; ///< Requested color format.
        VkFormat m_depthFormat = VK_FORMAT_UNDEFINED; ///< Requested depth format.
        VkFormat m_format = VK_FORMAT_UNDEFINED; ///< Selected surface color format.
        VkExtent2D m_extent{}; ///< Dimensions of swapchain images.

        std::vector<Image> m_images; ///< Wrapped swapchain color images.
        std::unique_ptr<Image> m_depthImage; ///< Dedicated depth image attachment.
        std::vector<Framebuffer> m_framebuffers; ///< Framebuffers bound to color + depth images.

        size_t m_currentFrame = 0; ///< Index of current active frame slot (0..MAX_FRAMES_IN_FLIGHT - 1).

        // Synchronization Primitives (Per Frame in Flight)
        std::vector<Semaphore> m_imageAvailable; ///< Semaphores signaled when a frame's image is ready for rendering.
        std::vector<Fence> m_inFlightFences; ///< Fences preventing CPU from overwriting command buffers in use by GPU.

        // Synchronization Primitives (Per Swapchain Image)
        std::vector<Semaphore> m_renderFinished;
        ///< Semaphores signaled when rendering to a specific image is complete.
        std::vector<VkFence> m_imagesInFlight;
        ///< Tracks which CPU-GPU fence is currently rendering into each swapchain image.
    };
} // namespace LavaVK

#endif // LAVAVK_SWAPCHAIN_HPP