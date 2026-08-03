#include <LavaVK/LavaVK.hpp>
#include <GLFW/glfw3.h>

int main() {
    constexpr uint32_t WIDTH = 1280;
    constexpr uint32_t HEIGHT = 720;

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow *window = glfwCreateWindow(
        WIDTH, HEIGHT,
        "LavaVK Example: Hello triangle!",
        nullptr, nullptr
    );

    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    uint32_t extensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    LavaVK::Instance instance({
        .applicationName = "LavaVK Example: Hello triangle!",
        .extensions = std::vector<const char *>(glfwExtensions, glfwExtensions + extensionCount)
    });

    LavaVK::Surface surface(instance, [window](VkInstance vkInst) -> VkSurfaceKHR {
        VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
        if (glfwCreateWindowSurface(vkInst, window, nullptr, &rawSurface) != VK_SUCCESS) {
            return VK_NULL_HANDLE;
        }
        return rawSurface;
    });
    // GPU Selection & Device Creation
    const auto &selectedGPU = LavaVK::GPUHardware::selectOptimalGPU(instance, surface);

    // Device automatically creates command pools for all passed QueueTypes
    LavaVK::Device device(
        selectedGPU,
        {LavaVK::QueueType::GRAPHICS, LavaVK::QueueType::PRESENT},
        &surface
    );

    auto setLayout = LavaVK::DescriptorSetLayout::Builder(device)
            .addBinding(0, LavaVK::DescriptorType::UniformBuffer, LavaVK::STAGE_VERTEX_BIT)
            .addBinding(1, LavaVK::DescriptorType::CombinedImageSampler, LavaVK::STAGE_FRAGMENT_BIT)
            .build();

    // Pipeline Layout
    LavaVK::PushConstantRange pushConstantRange{
        .stageFlags = LavaVK::STAGE_VERTEX_BIT,
        .offset = 0,
        .size = 64
    };

    std::vector<const LavaVK::DescriptorSetLayout *> layouts = {setLayout.get()};
    std::vector<LavaVK::PushConstantRange> pushConstants = {pushConstantRange};

    LavaVK::PipelineLayout pipelineLayout(device, layouts, pushConstants);

    // Render Pass
    LavaVK::RenderPass renderPass(
        device, LavaVK::Format(LavaVK::ChannelOrder::BGRA, LavaVK::BitDepth::B8, LavaVK::NumericType::Srgb),
        LavaVK::Format(LavaVK::ChannelOrder::D, LavaVK::BitDepth::B32, LavaVK::NumericType::Float));

    // Note: If your using glsl, you don't need to compile to SPIR-V
    // because LavaVK automatically compiles it you you.
    LavaVK::Shader vertexShader(device, "shader/main.vert");
    LavaVK::Shader fragmentShader(device, "shader/main.frag");

    LavaVK::GraphicsPipeline pipeline(
        device,
        {
            .vertexShader = &vertexShader,
            .fragmentShader = &fragmentShader,

            .layout = &pipelineLayout,
            .renderPass = &renderPass,

            .topology = LavaVK::Topology::TRIANGLES,

            .polygonMode = LavaVK::PolygonMode::FILL,
            .cullMode = LavaVK::CullMode::NONE,
            .frontFace = LavaVK::FrontFace::COUNTER_CLOCKWISE,

            .depthTest = true,
            .depthWrite = true,

            .blending = false
        });

    LavaVK::SwapChain swapchain(
        device,
        surface,
        renderPass,
        renderPass.getColorFormat(),
        renderPass.getDepthFormat(),
        {WIDTH, HEIGHT}
    );

    device.getCommandPool(LavaVK::QueueType::GRAPHICS).allocate(LavaVK::MAX_FRAMES_IN_FLIGHT);
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Acquire Next Swapchain Image
        uint32_t imageIndex = 0;
        LavaVK::Result acquireResult = swapchain.acquireImage(imageIndex);

        // If fails means that the window probably resized
        if (!acquireResult) {
            swapchain.recreate();
            continue;
        }

        size_t frameIndex = swapchain.currentFrame();

        LavaVK::CommandBuffer &cmdBuffer = device.getCommandPool(LavaVK::QueueType::GRAPHICS).retrieve(frameIndex);
        cmdBuffer.record(
            renderPass,
            swapchain.framebuffer(imageIndex),
            swapchain.extent(),
            [&](LavaVK::CommandBuffer &cmd) {
                cmd.bindPipeline(pipeline);
                cmd.draw(3);
            }
        );

        device.submit(
            LavaVK::QueueType::GRAPHICS,
            frameIndex,
            {swapchain.imageAvailableSemaphore()}, // Wait semaphore
            {LavaVK::PipelineStage::ColorAttachmentOutput}, // Wait stage
            {swapchain.renderFinishedSemaphore(imageIndex)}, // Signal semaphore
            &swapchain.inFlightFence() // Fence pointer
        );
        swapchain.present(imageIndex);
    }
    device.waitIdle();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
