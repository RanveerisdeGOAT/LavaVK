#include <LavaVK/LavaVK.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <chrono>

struct PushConstants {
    glm::vec4 cameraPos; // xyz = position, w = time
    glm::vec4 cameraDir; // xyz = normalized forward dir, w = aspect ratio
};

// --- FPS Camera Controller ---
struct Camera {
    glm::vec3 position{0.0f, 0.0f, -3.0f};

    // Yaw = 90° looks along +Z axis towards positive space
    float yaw{90.0f};
    float pitch{0.0f};

    float moveSpeed{5.0f};
    float mouseSensitivity{0.1f};

    bool firstMouse{true};
    bool cursorCaptured{true};
    double lastX{1280.0 / 2.0};
    double lastY{720.0 / 2.0};

    // Calculate normalized forward direction vector from Pitch & Yaw
    glm::vec3 getForward() const {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        return glm::normalize(front);
    }

    // Calculate right movement vector (perpendicular to forward and world up)
    glm::vec3 getRight() const {
        return glm::normalize(glm::cross(getForward(), glm::vec3(0.0f, 1.0f, 0.0f)));
    }

    void processKeyboard(GLFWwindow *window, float deltaTime) {
        float velocity = moveSpeed * deltaTime;
        glm::vec3 forward = getForward();
        glm::vec3 right = getRight();

        // WASD Controls
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) position += forward * velocity;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) position -= forward * velocity;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) position -= right * velocity;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) position += right * velocity;

        // Vertical Elevation
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) position.y += velocity;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) position.y -= velocity;
    }

    void processMouse(double xpos, double ypos) {
        if (!cursorCaptured) return;

        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = static_cast<float>(xpos - lastX) * mouseSensitivity;
        float yoffset = static_cast<float>(lastY - ypos) * mouseSensitivity; // Reversed: Y goes top-to-bottom

        lastX = xpos;
        lastY = ypos;

        yaw += xoffset;
        pitch -= yoffset;

        // Clamp pitch so the camera doesn't flip upside down
        pitch = glm::clamp(pitch, -89.0f, 89.0f);
    }
};

// Global camera instance for GLFW callbacks
Camera g_camera;

void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    // Toggle mouse capture with ESC
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        g_camera.cursorCaptured = !g_camera.cursorCaptured;
        if (g_camera.cursorCaptured) {
            g_camera.firstMouse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void cursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
    g_camera.processMouse(xpos, ypos);
}

int main() {
    constexpr uint32_t WIDTH = 1280;
    constexpr uint32_t HEIGHT = 720;

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "LavaVK: Compute Raycaster", nullptr, nullptr);
    if (!window) return -1;

    // Capture mouse cursor for 360-degree rotation
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Register Callbacks
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);

    uint32_t extensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    LavaVK::Instance instance({
        .applicationName = "LavaVK Compute Sphere Raycast",
        .extensions = std::vector<const char *>(glfwExtensions, glfwExtensions + extensionCount)
    });

    LavaVK::Surface surface(instance, [window](VkInstance vkInst) -> VkSurfaceKHR {
        VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
        glfwCreateWindowSurface(vkInst, window, nullptr, &rawSurface);
        return rawSurface;
    });

    LavaVK::Device device(
        LavaVK::GPUHardware::selectOptimalGPU(instance, surface),
        {LavaVK::QueueType::COMPUTE, LavaVK::QueueType::GRAPHICS, LavaVK::QueueType::PRESENT},
        &surface
    );

    // Dummy RenderPass to satisfy SwapChain constructor parameters
    LavaVK::RenderPass renderPass(device, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_D32_SFLOAT);

    // SwapChain Initialization
    LavaVK::SwapChain swapchain(
        device,
        surface,
        renderPass,
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_D32_SFLOAT,
        VkExtent2D{WIDTH, HEIGHT}
    );

    // --- Storage Image for Raytracer Output ---
    LavaVK::Texture renderTarget(device, {
        .width = WIDTH,
        .height = HEIGHT,
        .format = LavaVK::Format(LavaVK::ChannelOrder::RGBA, LavaVK::BitDepth::B8, LavaVK::NumericType::Unorm),
        .usage = LavaVK::ImageUsage::STORAGE | LavaVK::ImageUsage::SAMPLED | LavaVK::ImageUsage::TRANSFER_SRC
    });

    // --- Descriptors & Layout ---
    auto descriptorSetLayout = LavaVK::DescriptorSetLayout::Builder(device)
            .addBinding(0, LavaVK::DescriptorType::StorageImage, LavaVK::STAGE_COMPUTE_BIT)
            .build();

    auto descriptorPool = LavaVK::DescriptorPool::Builder(device)
            .setMaxSets(1)
            .addPoolSize(LavaVK::DescriptorType::StorageImage, 1)
            .build();

    LavaVK::DescriptorImage imageDescriptor(renderTarget, LavaVK::ImageLayout::GENERAL);

    LavaVK::DescriptorSet descriptorSet = descriptorPool->write(*descriptorSetLayout)
            .writeImage(0, &imageDescriptor)
            .build();

    // --- Pipeline Layout & Compute Pipeline ---
    LavaVK::PipelineLayout pipelineLayout(device, {descriptorSetLayout.get()}, {
        {
            .stageFlags = LavaVK::STAGE_COMPUTE_BIT,
            .offset = 0,
            .size = sizeof(PushConstants)
        }
    });

    LavaVK::Shader computeShader(device, "../shader/raycast.comp");

    LavaVK::ComputePipeline computePipeline(device, {
        .computeShader = &computeShader,
        .layout = &pipelineLayout
    });

    device.getCommandPool(LavaVK::QueueType::GRAPHICS).allocate(LavaVK::MAX_FRAMES_IN_FLIGHT);

    auto startTime = std::chrono::high_resolution_clock::now();
    auto lastFrameTime = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Calculate Delta Time & Elapsed Time
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
        float time = std::chrono::duration<float>(currentTime - startTime).count();
        lastFrameTime = currentTime;

        // Process Keyboard Movement
        g_camera.processKeyboard(window, deltaTime);

        // 1. Acquire Image Index and acquire signals
        uint32_t imageIndex = 0;
        if (swapchain.acquireImage(imageIndex) == VK_ERROR_OUT_OF_DATE_KHR) {
            swapchain.recreate();
            continue;
        }

        LavaVK::CommandBuffer &cmdBuffer = device.getCommandPool(LavaVK::QueueType::GRAPHICS).retrieve(
            swapchain.currentFrame());

        cmdBuffer.record([&](LavaVK::CommandBuffer &cmd) {
            // A. Dispatch Compute Raytracer Shader
            cmd.bindPipeline(computePipeline);
            cmd.bindDescriptorSets(pipelineLayout, LavaVK::PipelineBindPoint::Compute, {descriptorSet}, 0);

            // Pass updated camera position and forward direction vector to shader
            glm::vec3 camDir = g_camera.getForward();
            PushConstants pushData{
                .cameraPos = glm::vec4(g_camera.position, time),
                .cameraDir = glm::vec4(camDir, static_cast<float>(WIDTH) / static_cast<float>(HEIGHT))
            };
            cmd.pushConstants(pipelineLayout, LavaVK::STAGE_COMPUTE_BIT, pushData);

            uint32_t groupX = (WIDTH + 7) / 8;
            uint32_t groupY = (HEIGHT + 7) / 8;
            cmd.dispatch(groupX, groupY, 1);

            // B. Transition Swapchain Image: UNDEFINED -> TRANSFER_DST_OPTIMAL
            cmd.pipelineBarrier(
                swapchain.image(imageIndex),
                LavaVK::ImageLayout::UNDEFINED,
                LavaVK::ImageLayout::TRANSFER_DST_OPTIMAL,
                LavaVK::PipelineStage::TopOfPipe,
                LavaVK::PipelineStage::Transfer,
                LavaVK::AccessFlagBits::NONE,
                LavaVK::AccessFlagBits::TRANSFER_WRITE_BIT
            );

            // C. Transition RenderTarget Image: GENERAL -> TRANSFER_SRC_OPTIMAL
            cmd.pipelineBarrier(
                renderTarget.image(),
                LavaVK::ImageLayout::GENERAL,
                LavaVK::ImageLayout::TRANSFER_SRC_OPTIMAL,
                LavaVK::PipelineStage::ComputeShader,
                LavaVK::PipelineStage::Transfer,
                LavaVK::AccessFlagBits::SHADER_WRITE_BIT,
                LavaVK::AccessFlagBits::TRANSFER_READ_BIT
            );

            // D. Copy renderTarget -> swapchainVkImage
            cmd.copyImage(
                renderTarget.image(),
                LavaVK::ImageLayout::TRANSFER_SRC_OPTIMAL,
                swapchain.image(imageIndex),
                LavaVK::ImageLayout::TRANSFER_DST_OPTIMAL,
                WIDTH,
                HEIGHT
            );

            // E. Transition RenderTarget back: TRANSFER_SRC_OPTIMAL -> GENERAL
            cmd.pipelineBarrier(
                renderTarget.image(),
                LavaVK::ImageLayout::TRANSFER_SRC_OPTIMAL,
                LavaVK::ImageLayout::GENERAL,
                LavaVK::PipelineStage::Transfer,
                LavaVK::PipelineStage::ComputeShader,
                LavaVK::AccessFlagBits::TRANSFER_READ_BIT,
                LavaVK::AccessFlagBits::SHADER_WRITE_BIT | LavaVK::AccessFlagBits::SHADER_READ_BIT
            );

            // F. Transition Swapchain Image: TRANSFER_DST_OPTIMAL -> PRESENT_SRC_KHR
            cmd.pipelineBarrier(
                swapchain.image(imageIndex),
                LavaVK::ImageLayout::TRANSFER_DST_OPTIMAL,
                LavaVK::ImageLayout::PRESENT_SRC_KHR,
                LavaVK::PipelineStage::Transfer,
                LavaVK::PipelineStage::BottomOfPipe,
                LavaVK::AccessFlagBits::TRANSFER_WRITE_BIT,
                LavaVK::AccessFlagBits::NONE
            );
        });

        // 2. Submit with swapchain semaphores & in-flight fence
        device.submit(
            LavaVK::QueueType::GRAPHICS,
            swapchain.currentFrame(),
            {swapchain.imageAvailableSemaphore()},
            {LavaVK::PipelineStage::ColorAttachmentOutput},
            {swapchain.renderFinishedSemaphore(imageIndex)},
            &swapchain.inFlightFence()
        );

        // 3. Queue Present
        swapchain.present(imageIndex);
    }

    device.waitIdle();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}