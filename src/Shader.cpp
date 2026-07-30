#include "LavaVK/Shader.hpp"
#include "LavaVK/Device.hpp"
#include "LavaVK/Error.hpp"


namespace LavaVK {
    namespace {
        // Helper to deduce ShaderType from file extension
        LavaVK::ShaderType deduceShaderType(const std::string &filepath) {
            std::filesystem::path path(filepath);
            std::string ext = path.extension().string();

            if (ext == ".vert") return LavaVK::ShaderType::Vertex;
            if (ext == ".frag") return LavaVK::ShaderType::Fragment;
            if (ext == ".comp") return LavaVK::ShaderType::Compute;

            LAVAVK_ERROR("[LavaVK Error] Unknown shader extension: " + ext);
        }

        std::vector<uint32_t> compileGLSLToSPIRV(
            const std::string &filepath,
            LavaVK::ShaderType type) {
            // Read source code from file
            std::ifstream file(filepath);
            if (!file.is_open()) {
                LAVAVK_ERROR("[Shaderc Error] Failed to open GLSL file: " + filepath);
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string sourceCode = buffer.str();

            shaderc::Compiler compiler;
            shaderc::CompileOptions options;

            options.SetOptimizationLevel(shaderc_optimization_level_performance);
            options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);

            shaderc_shader_kind kind;
            switch (type) {
                case LavaVK::ShaderType::Vertex: kind = shaderc_glsl_vertex_shader;
                    break;
                case LavaVK::ShaderType::Fragment: kind = shaderc_glsl_fragment_shader;
                    break;
                case LavaVK::ShaderType::Compute: kind = shaderc_glsl_compute_shader;
                    break;
            }

            shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
                sourceCode,
                kind,
                filepath.c_str(),
                options
            );

            if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
                LAVAVK_ERROR("[Shaderc Error] " + module.GetErrorMessage());
            }

            return {module.cbegin(), module.cend()};
        }

        static std::vector<uint32_t> readSpvFile(const std::string &filename) {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);
            if (!file.is_open()) {
                LAVAVK_ERROR("[LavaVK ERROR] Failed to open shader file: " + filename);
            }

            size_t fileSize = static_cast<size_t>(file.tellg());
            if (fileSize % sizeof(uint32_t) != 0) {
                LAVAVK_ERROR(
                    "[LavaVK ERROR] Invalid SPIR-V file size (must be a multiple of 4 bytes): " + filename);
            }

            std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
            file.seekg(0);
            file.read(reinterpret_cast<char *>(buffer.data()), fileSize);
            file.close();

            return buffer;
        }
    }

    Shader::Shader(Device &device, const std::string &filepath)
        : m_device(device) {
        std::filesystem::path path(filepath);

        if (path.extension() == ".spv") {
            std::vector<uint32_t> spirv = readSpvFile(filepath);
            createShaderModule(spirv);
        } else {
            ShaderType type = deduceShaderType(filepath);
            std::vector<uint32_t> spirv = compileGLSLToSPIRV(filepath, type);
            createShaderModule(spirv);
        }
    }

    Shader::Shader(Device &device, const std::vector<uint32_t> &code)
        : m_device(device) {
        createShaderModule(code);
    }

    Shader::~Shader() {
        if (m_module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_device.native(), m_module, nullptr);
        }
    }

    void Shader::createShaderModule(const std::vector<uint32_t> &code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        // codeSize MUST BE IN BYTES
        createInfo.codeSize = code.size() * sizeof(uint32_t);
        // Properly aligned pointer
        createInfo.pCode = code.data();

        if (vkCreateShaderModule(m_device.native(), &createInfo, nullptr, &m_module) != VK_SUCCESS) {
            LAVAVK_ERROR("[LavaVK ERROR] Failed to create shader module!");
        }
    }
}
