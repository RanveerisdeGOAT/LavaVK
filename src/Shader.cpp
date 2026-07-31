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

            std::filesystem::path output =
                    std::filesystem::temp_directory_path() /
                    (std::filesystem::path(filepath).filename().string() + ".spv");

            std::string command =
                    "glslc \"" + filepath +
                    "\" -o \"" + output.string() + "\"";

            int result = std::system(command.c_str());

            if (result != 0) {
                LAVAVK_ERROR("[LavaVK ERROR] Failed to compile shader.");
            }

            std::ifstream file_output(output, std::ios::binary | std::ios::ate);

            if (!file_output)
                LAVAVK_ERROR("Failed to open compiled SPIR-V.");

            size_t size = static_cast<size_t>(file_output.tellg());

            if (size % sizeof(uint32_t) != 0)
                LAVAVK_ERROR("Invalid SPIR-V file.");

            file_output.seekg(0);

            std::vector<uint32_t> spirv(size / sizeof(uint32_t));

            file_output.read(
                reinterpret_cast<char*>(spirv.data()),
                size
            );

            return spirv;
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
