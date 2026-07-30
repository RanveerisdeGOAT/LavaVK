#include <LavaVK/LavaVK.hpp>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>
#include <chrono>

// Vertex structure with Texture Coordinates
struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoord;
};

int main() {
    constexpr uint32_t WIDTH = 1280;
    constexpr uint32_t HEIGHT = 720;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow *window = glfwCreateWindow(
        WIDTH, HEIGHT,
        "LavaVK Example: Textured Cube",
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
        .applicationName = "LavaVK Example: Textured Cube",
        .extensions = std::vector<const char *>(glfwExtensions, glfwExtensions + extensionCount)
    });

    LavaVK::Surface surface(instance, [window](VkInstance vkInst) -> VkSurfaceKHR {
        VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
        if (glfwCreateWindowSurface(vkInst, window, nullptr, &rawSurface) != VK_SUCCESS) {
            return VK_NULL_HANDLE;
        }
        return rawSurface;
    });

    const auto &selectedGPU = LavaVK::GPUHardware::selectOptimalGPU(instance, surface);

    LavaVK::Device device(
        selectedGPU,
        {LavaVK::QueueType::GRAPHICS, LavaVK::QueueType::PRESENT},
        &surface
    );

    // 24 Vertices (4 per face) with 0.0 to 1.0 UV mapping
    const std::vector<Vertex> vertices = {
        // Front face
        {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}},

        // Back face
        {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}},

        // Top face
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f}},

        // Bottom face
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}},

        // Right face
        {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}},

        // Left face
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}}
    };

    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0, // Front
        4, 5, 6, 6, 7, 4, // Back
        8, 9, 10, 10, 11, 8, // Top
        12, 13, 14, 14, 15, 12, // Bottom
        16, 17, 18, 18, 19, 16, // Right
        20, 21, 22, 22, 23, 20 // Left
    };


    // Load Dirt Texture
    LavaVK::Texture texture(device, "../assets/dirt.png", {
                                .magFilter = LavaVK::Filter::NEAREST,
                                .minFilter = LavaVK::Filter::NEAREST,
                            });

    // Build DescriptorSetLayout using LavaVK::DescriptorSetLayout::Builder
    auto descriptorSetLayout = LavaVK::DescriptorSetLayout::Builder(device)
            .addBinding(0, LavaVK::DescriptorType::CombinedImageSampler, LavaVK::STAGE_FRAGMENT_BIT)
            .build();

    // Build DescriptorPool using LavaVK::DescriptorPool::Builder
    auto descriptorPool = LavaVK::DescriptorPool::Builder(device)
            .setMaxSets(1)
            .addPoolSize(LavaVK::DescriptorType::CombinedImageSampler, 1)
            .build();

    LavaVK::DescriptorImage imageDescriptor(texture, LavaVK::ImageLayout::SHADER_READ_ONLY_OPTIMAL);

    // 2. Allocate, write, and return the DescriptorSet directly
    LavaVK::DescriptorSet descriptorSet = descriptorPool->write(*descriptorSetLayout)
            .writeImage(0, &imageDescriptor)
            .build();

    // Create Vertex & Index Buffers
    LavaVK::Buffer vertexBuffer(device, {
                                    .size = sizeof(Vertex) * vertices.size(),
                                    .usage = LavaVK::BufferUsage::Vertex,
                                    .memory = LavaVK::MemoryUsage::CPU_TO_GPU
                                });
    vertexBuffer.upload(vertices);

    LavaVK::Buffer indexBuffer(device, {
                                   .size = sizeof(uint16_t) * indices.size(),
                                   .usage = LavaVK::BufferUsage::Index,
                                   .memory = LavaVK::MemoryUsage::CPU_TO_GPU
                               });
    indexBuffer.upload(indices);

    // Build Vertex Layout
    LavaVK::VertexLayout vertexLayout = LavaVK::VertexLayout::create<Vertex>()
            .attribute<&Vertex::position>(
                0, LavaVK::Format(LavaVK::ChannelOrder::RGB, LavaVK::BitDepth::B32, LavaVK::NumericType::Float))
            .attribute<&Vertex::texCoord>(
                1, LavaVK::Format(LavaVK::ChannelOrder::RG, LavaVK::BitDepth::B32, LavaVK::NumericType::Float));

    // Pass descriptor set layout pointer to PipelineLayout
    LavaVK::PipelineLayout pipelineLayout(device, {descriptorSetLayout.get()}, {
                                              {
                                                  .stageFlags = LavaVK::STAGE_VERTEX_BIT,
                                                  .offset = 0,
                                                  .size = sizeof(glm::mat4)
                                              }
                                          });

    LavaVK::RenderPass renderPass(
        device,
        LavaVK::Format(LavaVK::ChannelOrder::BGRA, LavaVK::BitDepth::B8, LavaVK::NumericType::Srgb),
        LavaVK::Format(LavaVK::ChannelOrder::D, LavaVK::BitDepth::B32, LavaVK::NumericType::Float)
    );

    LavaVK::Shader vertexShader(device, "shader/tri.vert");
    LavaVK::Shader fragmentShader(device, "shader/tri.frag");

    LavaVK::GraphicsPipeline pipeline(
        device,
        {
            .vertexShader = &vertexShader,
            .fragmentShader = &fragmentShader,
            .layout = &pipelineLayout,
            .renderPass = &renderPass,
            .vertexLayout = &vertexLayout,
            .topology = LavaVK::Topology::TRIANGLES,
            .polygonMode = LavaVK::PolygonMode::FILL,
            .cullMode = LavaVK::CullMode::BACK,
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

    auto startTime = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        uint32_t imageIndex = 0;
        LavaVK::Result acquireResult = swapchain.acquireImage(imageIndex);

        if (!acquireResult) {
            swapchain.recreate();
            continue;
        }

        size_t frameIndex = swapchain.currentFrame();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        // Calculate MVP Matrix
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.5f, 1.0f, 0.0f));
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.5f), glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));

        float aspectRatio = static_cast<float>(swapchain.extent().width) / static_cast<float>(swapchain.extent().
                                height);
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 10.0f);
        proj[1][1] *= -1.0f; // Vulkan Y-axis flip

        glm::mat4 mvp = proj * view * model;

        // Record Commands
        LavaVK::CommandBuffer &cmdBuffer = device.getCommandPool(LavaVK::QueueType::GRAPHICS).retrieve(frameIndex);
        cmdBuffer.record(
            renderPass,
            swapchain.framebuffer(imageIndex),
            swapchain.extent(),
            [&](LavaVK::CommandBuffer &cmd) {
                cmd.bindPipeline(pipeline);
                cmd.setViewportAndScissor(swapchain.extent());

                // Bind Descriptor Set to pipeline via Vulkan API or cmd.bindDescriptorSet wrapper
                cmd.bindDescriptorSets(pipelineLayout, LavaVK::PipelineBindPoint::Graphics,
                                       {descriptorSet}, 0);

                // Push MVP Matrix to GPU
                cmd.pushConstants(
                    pipelineLayout,
                    LavaVK::STAGE_VERTEX_BIT,
                    mvp
                );

                // Bind Buffers & Draw
                cmd.bindVertexBuffer(vertexBuffer);
                cmd.bindIndexBuffer(indexBuffer);
                cmd.drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
            }
        );

        // Submit to GPU
        device.submit(
            LavaVK::QueueType::GRAPHICS,
            frameIndex,
            {swapchain.imageAvailableSemaphore()},
            {LavaVK::PipelineStage::ColorAttachmentOutput},
            {swapchain.renderFinishedSemaphore(imageIndex)},
            &swapchain.inFlightFence()
        );

        swapchain.present(imageIndex);
    }

    device.waitIdle();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
