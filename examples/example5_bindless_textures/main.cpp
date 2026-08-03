#include <LavaVK/LavaVK.hpp>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

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
    glm::vec4 textureId; // Only .x is used; packed as a float and rounded in the vertex shader
                          // so the existing RGBA/float32 vertex format can be reused as-is.
};

// Helper: copy a glm::mat4 into an InstanceData's 4 column vectors.
static void setInstanceModel(InstanceData &inst, const glm::mat4 &model) {
    std::memcpy(&inst.modelCol0, &model, sizeof(glm::mat4));
}

// Returns true if `path` has an extension recognized as an image file.
static bool isImageFile(const std::filesystem::path &path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
           ext == ".bmp" || ext == ".tga";
}

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
        "LavaVK Example: Bindless Textured Cubes",
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
        .applicationName = "LavaVK Example: Bindless Textured Cubes",
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

    // ---- Load every image in ../assets into the bindless texture array ----
    LavaVK::BindlessTextureSet bindlessTextures(device);

    std::vector<std::unique_ptr<LavaVK::Texture>> textures;
    std::vector<uint32_t> textureIds;

    const std::filesystem::path assetsDir = "../assets";
    for (const auto &entry : std::filesystem::directory_iterator(assetsDir)) {
        if (!entry.is_regular_file() || !isImageFile(entry.path())) {
            continue;
        }

        auto texture = std::make_unique<LavaVK::Texture>(
            device, entry.path(),
            LavaVK::TextureSamplerCreateInfo{
                .magFilter = LavaVK::Filter::NEAREST,
                .minFilter = LavaVK::Filter::NEAREST,
            });

        const uint32_t id = bindlessTextures.add(*texture);

        std::cout << "Loaded texture '" << entry.path().filename().string()
                  << "' as bindless id " << id << '\n';

        textureIds.push_back(id);
        textures.push_back(std::move(texture));
    }

    if (textures.empty()) {
        std::cerr << "No image files found in " << assetsDir << "\n";
        return -1;
    }

    // ---- Per-instance data: one cube per texture, laid out in a row ----
    const uint32_t instanceCount = static_cast<uint32_t>(textures.size());
    std::vector<InstanceData> instances(instanceCount);
    {
        constexpr float SPACING = 2.0f;
        const float offset = (static_cast<float>(instanceCount) - 1) * SPACING * 0.5f;

        for (uint32_t i = 0; i < instanceCount; ++i) {
            const glm::vec3 pos(i * SPACING - offset, 0.0f, 0.0f);

            const glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
            setInstanceModel(instances[i], model);

            instances[i].tintColor = glm::vec4(1.0f);
            instances[i].textureId = glm::vec4(static_cast<float>(textureIds[i]), 0.0f, 0.0f, 0.0f);
        }
    }

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
        .instanceCount = instanceCount, // One instance per loaded texture
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
                6, LavaVK::Format(LavaVK::ChannelOrder::RGBA, LavaVK::BitDepth::B32, LavaVK::NumericType::Float))
            .attribute<&InstanceData::textureId>(
                7, LavaVK::Format(LavaVK::ChannelOrder::RGBA, LavaVK::BitDepth::B32, LavaVK::NumericType::Float));

    // The pipeline's only descriptor set is the bindless texture array's layout.
    LavaVK::PipelineLayout pipelineLayout(device, {&bindlessTextures.layout()}, {
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
                                       {bindlessTextures.descriptorSet()}, 0);

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