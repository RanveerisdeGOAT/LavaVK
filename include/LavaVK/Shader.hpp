#ifndef LAVAVK_SHADER_HPP
#define LAVAVK_SHADER_HPP

#include <vulkan/vulkan_core.h>
#include "shaderc/shaderc.hpp"
#include <fstream>
#include <filesystem>

namespace LavaVK {
    class Device;
    /**
     * @brief Shader types supported by the LavaVK abstraction layer.
     */
    enum class ShaderType {
        Vertex, /**< Vertex shader (.vert or SPIR-V) */
        Fragment, /**< Fragment/Pixel shader (.frag or SPIR-V) */
        Compute /**< Compute shader (.comp or SPIR-V) */
    };

    /**
     * @brief Bitmask-style enum identifying which pipeline stage(s) a shader targets,
     * mirroring `VkShaderStageFlagBits`.
     * @details Combine multiple stages with the provided `operator|`/`operator|=`
     * overloads (e.g. for descriptor bindings visible to more than one stage).
     */
    enum class ShaderStage : VkShaderStageFlags {
        None = 0, /**< No stages. */
        Vertex = VK_SHADER_STAGE_VERTEX_BIT, /**< Vertex shader stage. */
        Fragment = VK_SHADER_STAGE_FRAGMENT_BIT, /**< Fragment shader stage. */
        Compute = VK_SHADER_STAGE_COMPUTE_BIT, /**< Compute shader stage. */
        Geometry = VK_SHADER_STAGE_GEOMETRY_BIT, /**< Geometry shader stage. */
        TessellationControl = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, /**< Tessellation control shader stage. */
        TesselationEvaluation = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, /**< Tessellation evaluation shader stage. */
        RayGeneration = VK_SHADER_STAGE_RAYGEN_BIT_KHR, /**< Ray generation shader stage. */
        All = VK_SHADER_STAGE_ALL, /**< All shader stages. */
    };

    /**
     * @brief Combines two shader stage flags.
     * @param a First set of stage flags.
     * @param b Second set of stage flags.
     * @return A #ShaderStage containing the bitwise OR of @p a and @p b.
     */
    inline ShaderStage operator|(ShaderStage a, ShaderStage b) {
        return static_cast<ShaderStage>(
            static_cast<VkShaderStageFlags>(a) | static_cast<VkShaderStageFlags>(b)
        );
    }

    /**
     * @brief Combines a shader stage flag into an existing set in place.
     * @param a Set of stage flags to modify; receives the combined result.
     * @param b Stage flags to add to @p a.
     * @return Reference to @p a after combining.
     */
    inline ShaderStage &operator|=(ShaderStage &a, ShaderStage b) {
        a = a | b;
        return a;
    }


    /**
     * @brief Encapsulates a Vulkan shader module (`VkShaderModule`).
     *
     * @details
     * Supports loading directly from raw SPIR-V binaries (`.spv`), raw SPIR-V
     * byte vectors, or dynamically compiling GLSL source files (`.vert`, `.frag`, `.comp`)
     * using embedded Shaderc compiler support. `Shader` is non-copyable RAII:
     * the underlying `VkShaderModule` is created in the constructor and
     * destroyed in the destructor.
     *
     * Example, compiling GLSL source on load:
     * @code
     * LavaVK::Shader vertexShader(device, "shaders/tri.vert");
     * LavaVK::Shader fragmentShader(device, "shaders/tri.frag");
     * @endcode
     *
     * Example, loading precompiled SPIR-V words directly:
     * @code
     * std::vector<uint32_t> spirv = loadSpirvWords("shaders/tri.vert.spv");
     * LavaVK::Shader vertexShader(device, spirv);
     * @endcode
     */
    class Shader {
    public:
        /**
         * @brief Loads and constructs a shader module from a file path.
         * @details Automatically detects file extension (`.spv` for compiled
         * SPIR-V, `.vert`/`.frag`/`.comp` for GLSL source, compiled at load
         * time via the embedded Shaderc compiler).
         * @param device Reference to the logical LavaVK device.
         * @param filepath Path to the shader file on disk.
         * @throws std::runtime_error If file opening, GLSL compilation, or
         * Vulkan shader module creation fails.
         */
        Shader(Device &device, const std::string &filepath);

        /**
         * @brief Constructs a shader module directly from a vector of SPIR-V 32-bit words.
         * @param device Reference to the logical LavaVK device.
         * @param code SPIR-V bytecode binary vector.
         * @throws std::runtime_error If Vulkan shader module creation fails.
         */
        Shader(Device &device, const std::vector<uint32_t> &code);

        /**
         * @brief Destroys the underlying `VkShaderModule`.
         */
        ~Shader();

        // Non-copyable
        Shader(const Shader &) = delete;

        Shader &operator=(const Shader &) = delete;

        /**
         * @brief Retrieves the native Vulkan `VkShaderModule` handle.
         * @return Raw VkShaderModule handle.
         */
        [[nodiscard]] VkShaderModule native() const { return m_module; }

    private:
        /**
         * @brief Internal helper to allocate the native Vulkan shader module.
         * @param code SPIR-V bytecode vector.
         * @throws std::runtime_error If `vkCreateShaderModule` fails.
         */
        void createShaderModule(const std::vector<uint32_t> &code);

        Device &m_device;
        VkShaderModule m_module{VK_NULL_HANDLE};
    };

    /**
     * @brief Bitflags specifying shader stage accessibility for descriptors and push constants.
     * @details A plain (non-`enum class`) bitmask alternative to #ShaderStage,
     * used where raw `uint32_t`-style OR'ing without casts is more convenient
     * (e.g. `LavaVK::STAGE_VERTEX_BIT` when declaring a `PushConstantRange`).
     * Convert to native Vulkan flags via #toVkShaderStageFlags().
     */
    enum ShaderStageFlags : uint32_t {
        STAGE_VERTEX_BIT = 0x00000001, /**< VK_SHADER_STAGE_VERTEX_BIT */
        STAGE_TESSELLATION_CONTROL_BIT = 0x00000002, /**< VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT */
        STAGE_TESSELLATION_EVALUATION_BIT = 0x00000004, /**< VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT */
        STAGE_GEOMETRY_BIT = 0x00000008, /**< VK_SHADER_STAGE_GEOMETRY_BIT */
        STAGE_FRAGMENT_BIT = 0x00000010, /**< VK_SHADER_STAGE_FRAGMENT_BIT */
        STAGE_COMPUTE_BIT = 0x00000020, /**< VK_SHADER_STAGE_COMPUTE_BIT */
        STAGE_ALL_GRAPHICS = 0x0000001F, /**< All graphics stages combined */
        STAGE_ALL = 0x7FFFFFFF /**< All pipeline stages */
    };

    /**
     * @brief Converts LavaVK's #ShaderStageFlags bitmask into native `VkShaderStageFlags`.
     * @param flags Bitwise-OR'd combination of #ShaderStageFlags values.
     * @return The equivalent `VkShaderStageFlags` bitmask, with each set
     * LavaVK bit mapped to its corresponding `VK_SHADER_STAGE_*_BIT` value.
     * Bits not individually handled (e.g. #STAGE_ALL_GRAPHICS, #STAGE_ALL)
     * do not contribute additional flags beyond their constituent bits.
     */
    static VkShaderStageFlags toVkShaderStageFlags(uint32_t flags) {
        VkShaderStageFlags result = 0;
        if (flags & STAGE_VERTEX_BIT) result |= VK_SHADER_STAGE_VERTEX_BIT;
        if (flags & STAGE_TESSELLATION_CONTROL_BIT) result |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        if (flags & STAGE_TESSELLATION_EVALUATION_BIT) result |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        if (flags & STAGE_GEOMETRY_BIT) result |= VK_SHADER_STAGE_GEOMETRY_BIT;
        if (flags & STAGE_FRAGMENT_BIT) result |= VK_SHADER_STAGE_FRAGMENT_BIT;
        if (flags & STAGE_COMPUTE_BIT) result |= VK_SHADER_STAGE_COMPUTE_BIT;
        return result;
    }
}

#endif //LAVAVK_SHADER_HPP