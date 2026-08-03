#include <LavaVK/LavaVK.hpp>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <cstring>

struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoord;
};

struct InstanceData {
    glm::vec4 modelCol0;
    glm::vec4 modelCol1;
    glm::vec4 modelCol2;
    glm::vec4 modelCol3;
    glm::vec4 tintColor;
};

// Helper: copy a glm::mat4 into an InstanceData's 4 column vectors.
static void setInstanceModel(InstanceData &inst, const glm::mat4 &model) {
    std::memcpy(&inst.modelCol0, &model, sizeof(glm::mat4));
}

constexpr uint32_t INSTANCE_COUNT = 1000000;

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
        "LavaVK Example: Instanced Textured Cubes",
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
        .applicationName = "LavaVK Example: Instanced Textured Cubes",
        .extensions = std::vector<const char *>(glfwExtensions, glfwExtensions + extensionCount)
    });

    LavaVK::Surface surface(instance, [window](VkInstance vkInst) -> VkSurfaceKHR {
        VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
        if (glfwCreateWindowSurface(vkInst, window, nullptr, &rawSurface) != VK_SUCCESS) {
            return VK_NULL_HANDLE;
        }
        return rawSurface;
    });

    LavaVK::Device device(
        LavaVK::GPUHardware::selectOptimalGPU(instance, surface),
        {LavaVK::QueueType::GRAPHICS, LavaVK::QueueType::PRESENT},
        &surface
    );

    // ---- Geometry: one cube, shared by every instance ----
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

    // ---- Per-instance data: scatter cubes in a grid with random tint ----
    std::vector<InstanceData> instances(INSTANCE_COUNT);
    {
        std::mt19937 rng(std::time(nullptr));
        std::uniform_real_distribution<float> colorDist(0.3f, 1.0f);

        constexpr int GRID_SIDE = 1000; // 1000x1000 = 1,000,000 cubes
        constexpr float SPACING = 1.0f;
        constexpr float offset = (GRID_SIDE - 1) * SPACING * 0.5f;

        for (uint32_t i = 0; i < INSTANCE_COUNT; ++i) {
            int gx = static_cast<int>(i) % GRID_SIDE;
            int gz = static_cast<int>(i) / GRID_SIDE;

            glm::vec3 pos(
                gx * SPACING - offset,
                0.0f,
                gz * SPACING - offset
            );

            glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
            setInstanceModel(instances[i], model);

            instances[i].tintColor = glm::vec4(
                colorDist(rng), colorDist(rng), colorDist(rng), 1.0f
            );
        }
    }

    // Load Dirt Texture
    LavaVK::Texture texture(device, "../assets/dirt.png", {
                                .magFilter = LavaVK::Filter::NEAREST,
                                .minFilter = LavaVK::Filter::NEAREST,
                            });

    auto descriptorSetLayout = LavaVK::DescriptorSetLayout::Builder(device)
            .addBinding(0, LavaVK::DescriptorType::CombinedImageSampler, LavaVK::STAGE_FRAGMENT_BIT)
            .build();

    auto descriptorPool = LavaVK::DescriptorPool::Builder(device)
            .setMaxSets(1)
            .addPoolSize(LavaVK::DescriptorType::CombinedImageSampler, 1)
            .build();

    LavaVK::DescriptorImage imageDescriptor(texture, LavaVK::ImageLayout::SHADER_READ_ONLY_OPTIMAL);

    LavaVK::DescriptorSet descriptorSet = descriptorPool->write(*descriptorSetLayout)
            .writeImage(0, &imageDescriptor)
            .build();

    // --- Vertex Buffer Staging ---
    LavaVK::BufferSize vertexBufferSize = sizeof(Vertex) * vertices.size();

    LavaVK::Buffer vertexStagingBuffer(device, {
        .size = vertexBufferSize,
        .usage = LavaVK::BufferUsage::TransferSrc,
        .memory = LavaVK::MemoryUsage::CPU_TO_GPU
    });
    vertexStagingBuffer.upload(vertices);

    LavaVK::Buffer vertexBuffer(device, {
        .size = vertexBufferSize,
        .usage = LavaVK::BufferUsage::Vertex | LavaVK::BufferUsage::TransferDst,
        .memory = LavaVK::MemoryUsage::GPU
    });
    vertexStagingBuffer.copyToBuffer(vertexBuffer);

    // --- Index Buffer Staging ---
    LavaVK::BufferSize indexBufferSize = sizeof(uint16_t) * indices.size();

    LavaVK::Buffer indexStagingBuffer(device, {
        .size = indexBufferSize,
        .usage = LavaVK::BufferUsage::TransferSrc,
        .memory = LavaVK::MemoryUsage::CPU_TO_GPU
    });
    indexStagingBuffer.upload(indices);

    LavaVK::Buffer indexBuffer(device, {
        .size = indexBufferSize,
        .usage = LavaVK::BufferUsage::Index | LavaVK::BufferUsage::TransferDst,
        .memory = LavaVK::MemoryUsage::GPU
    });
    indexStagingBuffer.copyToBuffer(indexBuffer);

    // --- Instance Buffer Staging ---
    LavaVK::BufferSize instanceBufferSize = sizeof(InstanceData) * instances.size();

    LavaVK::Buffer instanceStagingBuffer(device, {
        .size = instanceBufferSize,
        .usage = LavaVK::BufferUsage::TransferSrc,
        .memory = LavaVK::MemoryUsage::CPU_TO_GPU
    });
    instanceStagingBuffer.upload(instances);

    LavaVK::Buffer instanceBuffer(device, {
        .size = instanceBufferSize,
        .usage = LavaVK::BufferUsage::Vertex | LavaVK::BufferUsage::TransferDst,
        .memory = LavaVK::MemoryUsage::GPU
    });
    instanceStagingBuffer.copyToBuffer(instanceBuffer);

    // --- Indirect Draw Buffer Staging ---
    LavaVK::IndexedIndirectCommand indirectCmd{
        .indexCount    = static_cast<uint32_t>(indices.size()),
        .instanceCount = INSTANCE_COUNT, // Instancing enabled via indirect parameters
        .firstIndex    = 0,
        .vertexOffset  = 0,
        .firstInstance = 0,
    };

    LavaVK::BufferSize indirectBufferSize = sizeof(LavaVK::IndexedIndirectCommand);

    LavaVK::Buffer indirectStagingBuffer(device, {
        .size = indirectBufferSize,
        .usage = LavaVK::BufferUsage::TransferSrc,
        .memory = LavaVK::MemoryUsage::CPU_TO_GPU
    });
    indirectStagingBuffer.upload(indirectCmd);

    LavaVK::Buffer indirectBuffer(device, {
        .size = indirectBufferSize,
        .usage = LavaVK::BufferUsage::Indirect | LavaVK::BufferUsage::TransferDst,
        .memory = LavaVK::MemoryUsage::GPU
    });
    indirectStagingBuffer.copyToBuffer(indirectBuffer);

    LavaVK::VertexLayout vertexLayout = LavaVK::VertexLayout::create<Vertex>(0)
            .attribute<&Vertex::position>(
                0, LavaVK::Format(LavaVK::ChannelOrder::RGB, LavaVK::BitDepth::B32, LavaVK::NumericType::Float))
            .attribute<&Vertex::texCoord>(
                1, LavaVK::Format(LavaVK::ChannelOrder::RG, LavaVK::BitDepth::B32, LavaVK::NumericType::Float))
            .addBinding<InstanceData>(1, LavaVK::VertexInputRate::Instance)
            .attribute<&InstanceData::modelCol0>(
                2, LavaVK::Format(LavaVK::ChannelOrder::RGBA, LavaVK::BitDepth::B32, LavaVK::NumericType::Float))
            .attribute<&InstanceData::modelCol1>(
                3, LavaVK::Format(LavaVK::ChannelOrder::RGBA, LavaVK::BitDepth::B32, LavaVK::NumericType::Float))
            .attribute<&InstanceData::modelCol2>(
                4, LavaVK::Format(LavaVK::ChannelOrder::RGBA, LavaVK::BitDepth::B32, LavaVK::NumericType::Float))
            .attribute<&InstanceData::modelCol3>(
                5, LavaVK::Format(LavaVK::ChannelOrder::RGBA, LavaVK::BitDepth::B32, LavaVK::NumericType::Float))
            .attribute<&InstanceData::tintColor>(
                6, LavaVK::Format(LavaVK::ChannelOrder::RGBA, LavaVK::BitDepth::B32, LavaVK::NumericType::Float));

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

    LavaVK::Shader vertexShader(device, "../shader/main.vert");
    LavaVK::Shader fragmentShader(device, "../shader/main.frag");

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
    float time = std::chrono::duration<float>(0).count();
    float last = time;
    float counter = time;

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
        time = std::chrono::duration<float>(currentTime - startTime).count();
        if (counter > 1) {
            std::cout << 1/(time - last) << std::endl;
            counter = 0;
        }
        counter += time - last;
        last = time;

        float camAngle = time * 0.2f;
        glm::vec3 camPos(std::cos(camAngle) * 14.0f, std::sin(camAngle) * 10, std::sin(camAngle) * 14.0f);
        glm::mat4 view = glm::lookAt(camPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        float aspectRatio = static_cast<float>(swapchain.extent().width) / static_cast<float>(swapchain.extent().height);
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
        proj[1][1] *= -1.0f;

        glm::mat4 viewProj = proj * view;

        LavaVK::CommandBuffer &cmdBuffer = device.getCommandPool(LavaVK::QueueType::GRAPHICS).retrieve(frameIndex);
        cmdBuffer.record(
            renderPass,
            swapchain.framebuffer(imageIndex),
            swapchain.extent(),
            [&](LavaVK::CommandBuffer &cmd) {
                cmd.bindPipeline(pipeline);
                cmd.setViewportAndScissor(swapchain.extent());

                cmd.bindDescriptorSets(pipelineLayout, LavaVK::PipelineBindPoint::Graphics,
                                       {descriptorSet}, 0);

                cmd.pushConstants(
                    pipelineLayout,
                    LavaVK::STAGE_VERTEX_BIT,
                    viewProj
                );

                cmd.bindVertexBuffer(0, vertexBuffer, 0);
                cmd.bindVertexBuffer(1, instanceBuffer, 0);
                cmd.bindIndexBuffer(indexBuffer);

                cmd.drawIndexedIndirect(indirectBuffer, 0, 1, sizeof(LavaVK::IndexedIndirectCommand));
            }
        );

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