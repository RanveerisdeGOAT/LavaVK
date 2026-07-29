#ifndef LAVAVK_INSTANCE_H
#define LAVAVK_INSTANCE_H

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace LavaVK {
    /**
     * @brief Configuration parameters used to initialize a LavaVK Instance.
     */
    struct InstanceCreateInfo {
        /** @brief The name of the application. */
        std::string applicationName = "Application";

        /** @brief The version of the application encoded via VK_MAKE_VERSION. */
        uint32_t applicationVersion = VK_MAKE_VERSION(1, 0, 0);

        /** @brief Enables Khronos Vulkan validation layers in non-release builds (`#ifndef NDEBUG`). */
        bool enableValidation = true;

        /** @brief List of required Vulkan instance extensions. */
        std::vector<const char *> extensions;
    };

    /**
     * @brief Encapsulates a Vulkan instance (`VkInstance`) handle, managing its creation, lifetime, and cleanup.
     *
     * This class implements RAII semantics and move operations. Copying is explicitly disabled to prevent
     * multiple deletions of the underlying handle.
     */
    class Instance {
    public:
        /**
         * @brief Constructs a new LavaVK Instance and initializes the underlying Vulkan context.
         * * @param info Configuration structure specifying application metadata, validation rules, and extensions.
         * @throw std::runtime_error Thrown if `vkCreateInstance` fails to initialize the Vulkan context.
         */
        explicit Instance(const InstanceCreateInfo &info = {});

        /**
         * @brief Destructor. Destroys the managed `VkInstance` handle if valid.
         */
        ~Instance();

        /// @name Deleted Copy Operations
        /// @{
        Instance(const Instance &) = delete;

        Instance &operator=(const Instance &) = delete;

        /// @}

        /**
         * @brief Move constructor. Transfers ownership of the Vulkan instance handle from another `Instance`.
         * @param other The instance being moved from.
         */
        Instance(Instance &&other) noexcept;

        /**
         * @brief Move assignment operator. Destroys the current managed handle and acquires the handle from another `Instance`.
         * @param other The instance being moved from.
         * @return Reference to this instance.
         */
        Instance &operator=(Instance &&other) noexcept;

        /**
         * @brief Retrieves the raw, underlying native `VkInstance` handle.
         * @return The underlying `VkInstance` handle, or `VK_NULL_HANDLE` if invalid.
         */
        [[nodiscard]]
        VkInstance native() const {
            return m_instance;
        }

    private:
        /** @brief Native Vulkan instance handle. */
        VkInstance m_instance = VK_NULL_HANDLE;
    };

    enum class ChannelOrder {
        R,
        RG,
        RGB,
        BGR,
        RGBA,
        BGRA,
        D, // Depth
        DS, // Depth-Stencil
        Undefined
    };

    enum class BitDepth {
        B8,
        B16,
        B24,
        B32,
        Undefined
    };

    enum class NumericType {
        Unorm,
        Snorm,
        Uint,
        Sint,
        Float,
        Uscaled,
        Sscaled,
        Srgb,
        Undefined
    };

    class Format {
    public:
        // Default constructor creates VK_FORMAT_UNDEFINED
        constexpr Format()
            : channels(ChannelOrder::Undefined), bits(BitDepth::Undefined), type(NumericType::Undefined) {
        }

        constexpr Format(ChannelOrder channels, BitDepth bits, NumericType type)
            : channels(channels), bits(bits), type(type) {
        }

        // Construct directly from a raw VkFormat (converts Vulkan enum back to structured Format)
        constexpr Format(VkFormat format) {
            *this = format;
        }

        // Implicit conversion operator allows passing LavaVK::Format directly where VkFormat is expected
        constexpr operator VkFormat() const {
            return native();
        }

        // Assignment operator for Format
        constexpr Format &operator=(const Format &other) = default;

        // Assignment operator for native VkFormat
        constexpr Format &operator=(VkFormat format) {
            switch (format) {
                // R
                case VK_FORMAT_R8_UNORM: channels = ChannelOrder::R;
                    bits = BitDepth::B8;
                    type = NumericType::Unorm;
                    break;
                case VK_FORMAT_R8_SNORM: channels = ChannelOrder::R;
                    bits = BitDepth::B8;
                    type = NumericType::Snorm;
                    break;
                case VK_FORMAT_R8_UINT: channels = ChannelOrder::R;
                    bits = BitDepth::B8;
                    type = NumericType::Uint;
                    break;
                case VK_FORMAT_R8_SINT: channels = ChannelOrder::R;
                    bits = BitDepth::B8;
                    type = NumericType::Sint;
                    break;
                case VK_FORMAT_R8_SRGB: channels = ChannelOrder::R;
                    bits = BitDepth::B8;
                    type = NumericType::Srgb;
                    break;
                case VK_FORMAT_R16_UNORM: channels = ChannelOrder::R;
                    bits = BitDepth::B16;
                    type = NumericType::Unorm;
                    break;
                case VK_FORMAT_R16_SNORM: channels = ChannelOrder::R;
                    bits = BitDepth::B16;
                    type = NumericType::Snorm;
                    break;
                case VK_FORMAT_R16_UINT: channels = ChannelOrder::R;
                    bits = BitDepth::B16;
                    type = NumericType::Uint;
                    break;
                case VK_FORMAT_R16_SINT: channels = ChannelOrder::R;
                    bits = BitDepth::B16;
                    type = NumericType::Sint;
                    break;
                case VK_FORMAT_R16_SFLOAT: channels = ChannelOrder::R;
                    bits = BitDepth::B16;
                    type = NumericType::Float;
                    break;
                case VK_FORMAT_R32_UINT: channels = ChannelOrder::R;
                    bits = BitDepth::B32;
                    type = NumericType::Uint;
                    break;
                case VK_FORMAT_R32_SINT: channels = ChannelOrder::R;
                    bits = BitDepth::B32;
                    type = NumericType::Sint;
                    break;
                case VK_FORMAT_R32_SFLOAT: channels = ChannelOrder::R;
                    bits = BitDepth::B32;
                    type = NumericType::Float;
                    break;

                // RG
                case VK_FORMAT_R8G8_UNORM: channels = ChannelOrder::RG;
                    bits = BitDepth::B8;
                    type = NumericType::Unorm;
                    break;
                case VK_FORMAT_R8G8_SNORM: channels = ChannelOrder::RG;
                    bits = BitDepth::B8;
                    type = NumericType::Snorm;
                    break;
                case VK_FORMAT_R8G8_UINT: channels = ChannelOrder::RG;
                    bits = BitDepth::B8;
                    type = NumericType::Uint;
                    break;
                case VK_FORMAT_R8G8_SINT: channels = ChannelOrder::RG;
                    bits = BitDepth::B8;
                    type = NumericType::Sint;
                    break;
                case VK_FORMAT_R8G8_SRGB: channels = ChannelOrder::RG;
                    bits = BitDepth::B8;
                    type = NumericType::Srgb;
                    break;
                case VK_FORMAT_R16G16_UNORM: channels = ChannelOrder::RG;
                    bits = BitDepth::B16;
                    type = NumericType::Unorm;
                    break;
                case VK_FORMAT_R16G16_SNORM: channels = ChannelOrder::RG;
                    bits = BitDepth::B16;
                    type = NumericType::Snorm;
                    break;
                case VK_FORMAT_R16G16_UINT: channels = ChannelOrder::RG;
                    bits = BitDepth::B16;
                    type = NumericType::Uint;
                    break;
                case VK_FORMAT_R16G16_SINT: channels = ChannelOrder::RG;
                    bits = BitDepth::B16;
                    type = NumericType::Sint;
                    break;
                case VK_FORMAT_R16G16_SFLOAT: channels = ChannelOrder::RG;
                    bits = BitDepth::B16;
                    type = NumericType::Float;
                    break;
                case VK_FORMAT_R32G32_UINT: channels = ChannelOrder::RG;
                    bits = BitDepth::B32;
                    type = NumericType::Uint;
                    break;
                case VK_FORMAT_R32G32_SINT: channels = ChannelOrder::RG;
                    bits = BitDepth::B32;
                    type = NumericType::Sint;
                    break;
                case VK_FORMAT_R32G32_SFLOAT: channels = ChannelOrder::RG;
                    bits = BitDepth::B32;
                    type = NumericType::Float;
                    break;

                // RGB
                case VK_FORMAT_R8G8B8_UNORM: channels = ChannelOrder::RGB;
                    bits = BitDepth::B8;
                    type = NumericType::Unorm;
                    break;
                case VK_FORMAT_R8G8B8_SNORM: channels = ChannelOrder::RGB;
                    bits = BitDepth::B8;
                    type = NumericType::Snorm;
                    break;
                case VK_FORMAT_R8G8B8_UINT: channels = ChannelOrder::RGB;
                    bits = BitDepth::B8;
                    type = NumericType::Uint;
                    break;
                case VK_FORMAT_R8G8B8_SINT: channels = ChannelOrder::RGB;
                    bits = BitDepth::B8;
                    type = NumericType::Sint;
                    break;
                case VK_FORMAT_R8G8B8_SRGB: channels = ChannelOrder::RGB;
                    bits = BitDepth::B8;
                    type = NumericType::Srgb;
                    break;
                case VK_FORMAT_R16G16B16_UNORM: channels = ChannelOrder::RGB;
                    bits = BitDepth::B16;
                    type = NumericType::Unorm;
                    break;
                case VK_FORMAT_R16G16B16_SNORM: channels = ChannelOrder::RGB;
                    bits = BitDepth::B16;
                    type = NumericType::Snorm;
                    break;
                case VK_FORMAT_R16G16B16_UINT: channels = ChannelOrder::RGB;
                    bits = BitDepth::B16;
                    type = NumericType::Uint;
                    break;
                case VK_FORMAT_R16G16B16_SINT: channels = ChannelOrder::RGB;
                    bits = BitDepth::B16;
                    type = NumericType::Sint;
                    break;
                case VK_FORMAT_R16G16B16_SFLOAT: channels = ChannelOrder::RGB;
                    bits = BitDepth::B16;
                    type = NumericType::Float;
                    break;
                case VK_FORMAT_R32G32B32_UINT: channels = ChannelOrder::RGB;
                    bits = BitDepth::B32;
                    type = NumericType::Uint;
                    break;
                case VK_FORMAT_R32G32B32_SINT: channels = ChannelOrder::RGB;
                    bits = BitDepth::B32;
                    type = NumericType::Sint;
                    break;
                case VK_FORMAT_R32G32B32_SFLOAT: channels = ChannelOrder::RGB;
                    bits = BitDepth::B32;
                    type = NumericType::Float;
                    break;

                // BGR
                case VK_FORMAT_B8G8R8_UNORM: channels = ChannelOrder::BGR;
                    bits = BitDepth::B8;
                    type = NumericType::Unorm;
                    break;
                case VK_FORMAT_B8G8R8_SNORM: channels = ChannelOrder::BGR;
                    bits = BitDepth::B8;
                    type = NumericType::Snorm;
                    break;
                case VK_FORMAT_B8G8R8_UINT: channels = ChannelOrder::BGR;
                    bits = BitDepth::B8;
                    type = NumericType::Uint;
                    break;
                case VK_FORMAT_B8G8R8_SINT: channels = ChannelOrder::BGR;
                    bits = BitDepth::B8;
                    type = NumericType::Sint;
                    break;
                case VK_FORMAT_B8G8R8_SRGB: channels = ChannelOrder::BGR;
                    bits = BitDepth::B8;
                    type = NumericType::Srgb;
                    break;

                // RGBA
                case VK_FORMAT_R8G8B8A8_UNORM: channels = ChannelOrder::RGBA;
                    bits = BitDepth::B8;
                    type = NumericType::Unorm;
                    break;
                case VK_FORMAT_R8G8B8A8_SNORM: channels = ChannelOrder::RGBA;
                    bits = BitDepth::B8;
                    type = NumericType::Snorm;
                    break;
                case VK_FORMAT_R8G8B8A8_UINT: channels = ChannelOrder::RGBA;
                    bits = BitDepth::B8;
                    type = NumericType::Uint;
                    break;
                case VK_FORMAT_R8G8B8A8_SINT: channels = ChannelOrder::RGBA;
                    bits = BitDepth::B8;
                    type = NumericType::Sint;
                    break;
                case VK_FORMAT_R8G8B8A8_SRGB: channels = ChannelOrder::RGBA;
                    bits = BitDepth::B8;
                    type = NumericType::Srgb;
                    break;
                case VK_FORMAT_R16G16B16A16_UNORM: channels = ChannelOrder::RGBA;
                    bits = BitDepth::B16;
                    type = NumericType::Unorm;
                    break;
                case VK_FORMAT_R16G16B16A16_SNORM: channels = ChannelOrder::RGBA;
                    bits = BitDepth::B16;
                    type = NumericType::Snorm;
                    break;
                case VK_FORMAT_R16G16B16A16_UINT: channels = ChannelOrder::RGBA;
                    bits = BitDepth::B16;
                    type = NumericType::Uint;
                    break;
                case VK_FORMAT_R16G16B16A16_SINT: channels = ChannelOrder::RGBA;
                    bits = BitDepth::B16;
                    type = NumericType::Sint;
                    break;
                case VK_FORMAT_R16G16B16A16_SFLOAT: channels = ChannelOrder::RGBA;
                    bits = BitDepth::B16;
                    type = NumericType::Float;
                    break;
                case VK_FORMAT_R32G32B32A32_UINT: channels = ChannelOrder::RGBA;
                    bits = BitDepth::B32;
                    type = NumericType::Uint;
                    break;
                case VK_FORMAT_R32G32B32A32_SINT: channels = ChannelOrder::RGBA;
                    bits = BitDepth::B32;
                    type = NumericType::Sint;
                    break;
                case VK_FORMAT_R32G32B32A32_SFLOAT: channels = ChannelOrder::RGBA;
                    bits = BitDepth::B32;
                    type = NumericType::Float;
                    break;

                // BGRA
                case VK_FORMAT_B8G8R8A8_UNORM: channels = ChannelOrder::BGRA;
                    bits = BitDepth::B8;
                    type = NumericType::Unorm;
                    break;
                case VK_FORMAT_B8G8R8A8_SNORM: channels = ChannelOrder::BGRA;
                    bits = BitDepth::B8;
                    type = NumericType::Snorm;
                    break;
                case VK_FORMAT_B8G8R8A8_UINT: channels = ChannelOrder::BGRA;
                    bits = BitDepth::B8;
                    type = NumericType::Uint;
                    break;
                case VK_FORMAT_B8G8R8A8_SINT: channels = ChannelOrder::BGRA;
                    bits = BitDepth::B8;
                    type = NumericType::Sint;
                    break;
                case VK_FORMAT_B8G8R8A8_SRGB: channels = ChannelOrder::BGRA;
                    bits = BitDepth::B8;
                    type = NumericType::Srgb;
                    break;

                // Depth & Depth-Stencil
                case VK_FORMAT_D16_UNORM: channels = ChannelOrder::D;
                    bits = BitDepth::B16;
                    type = NumericType::Unorm;
                    break;
                case VK_FORMAT_D32_SFLOAT: channels = ChannelOrder::D;
                    bits = BitDepth::B32;
                    type = NumericType::Float;
                    break;
                case VK_FORMAT_D16_UNORM_S8_UINT: channels = ChannelOrder::DS;
                    bits = BitDepth::B16;
                    type = NumericType::Uint;
                    break;
                case VK_FORMAT_D24_UNORM_S8_UINT: channels = ChannelOrder::DS;
                    bits = BitDepth::B24;
                    type = NumericType::Uint;
                    break;
                case VK_FORMAT_D32_SFLOAT_S8_UINT: channels = ChannelOrder::DS;
                    bits = BitDepth::B32;
                    type = NumericType::Float;
                    break;

                default:
                    channels = ChannelOrder::Undefined;
                    bits = BitDepth::Undefined;
                    type = NumericType::Undefined;
                    break;
            }
            return *this;
        }

        /**
         * @brief Checks if the format represents an undefined state (VK_FORMAT_UNDEFINED).
         */
        [[nodiscard]] constexpr bool isUndefined() const {
            return native() == VK_FORMAT_UNDEFINED;
        }

        /**
         * @brief Evaluates the corresponding native VkFormat enum value.
         */
        [[nodiscard]] constexpr VkFormat native() const {
            // R Family
            if (channels == ChannelOrder::R) {
                if (bits == BitDepth::B8) {
                    if (type == NumericType::Unorm) return VK_FORMAT_R8_UNORM;
                    if (type == NumericType::Snorm) return VK_FORMAT_R8_SNORM;
                    if (type == NumericType::Uint) return VK_FORMAT_R8_UINT;
                    if (type == NumericType::Sint) return VK_FORMAT_R8_SINT;
                    if (type == NumericType::Srgb) return VK_FORMAT_R8_SRGB;
                }
                if (bits == BitDepth::B16) {
                    if (type == NumericType::Unorm) return VK_FORMAT_R16_UNORM;
                    if (type == NumericType::Snorm) return VK_FORMAT_R16_SNORM;
                    if (type == NumericType::Uint) return VK_FORMAT_R16_UINT;
                    if (type == NumericType::Sint) return VK_FORMAT_R16_SINT;
                    if (type == NumericType::Float) return VK_FORMAT_R16_SFLOAT;
                }
                if (bits == BitDepth::B32) {
                    if (type == NumericType::Uint) return VK_FORMAT_R32_UINT;
                    if (type == NumericType::Sint) return VK_FORMAT_R32_SINT;
                    if (type == NumericType::Float) return VK_FORMAT_R32_SFLOAT;
                }
            }

            // RG Family
            if (channels == ChannelOrder::RG) {
                if (bits == BitDepth::B8) {
                    if (type == NumericType::Unorm) return VK_FORMAT_R8G8_UNORM;
                    if (type == NumericType::Snorm) return VK_FORMAT_R8G8_SNORM;
                    if (type == NumericType::Uint) return VK_FORMAT_R8G8_UINT;
                    if (type == NumericType::Sint) return VK_FORMAT_R8G8_SINT;
                    if (type == NumericType::Srgb) return VK_FORMAT_R8G8_SRGB;
                }
                if (bits == BitDepth::B16) {
                    if (type == NumericType::Unorm) return VK_FORMAT_R16G16_UNORM;
                    if (type == NumericType::Snorm) return VK_FORMAT_R16G16_SNORM;
                    if (type == NumericType::Uint) return VK_FORMAT_R16G16_UINT;
                    if (type == NumericType::Sint) return VK_FORMAT_R16G16_SINT;
                    if (type == NumericType::Float) return VK_FORMAT_R16G16_SFLOAT;
                }
                if (bits == BitDepth::B32) {
                    if (type == NumericType::Uint) return VK_FORMAT_R32G32_UINT;
                    if (type == NumericType::Sint) return VK_FORMAT_R32G32_SINT;
                    if (type == NumericType::Float) return VK_FORMAT_R32G32_SFLOAT;
                }
            }

            // RGB Family
            if (channels == ChannelOrder::RGB) {
                if (bits == BitDepth::B8) {
                    if (type == NumericType::Unorm) return VK_FORMAT_R8G8B8_UNORM;
                    if (type == NumericType::Snorm) return VK_FORMAT_R8G8B8_SNORM;
                    if (type == NumericType::Uint) return VK_FORMAT_R8G8B8_UINT;
                    if (type == NumericType::Sint) return VK_FORMAT_R8G8B8_SINT;
                    if (type == NumericType::Srgb) return VK_FORMAT_R8G8B8_SRGB;
                }
                if (bits == BitDepth::B16) {
                    if (type == NumericType::Unorm) return VK_FORMAT_R16G16B16_UNORM;
                    if (type == NumericType::Snorm) return VK_FORMAT_R16G16B16_SNORM;
                    if (type == NumericType::Uint) return VK_FORMAT_R16G16B16_UINT;
                    if (type == NumericType::Sint) return VK_FORMAT_R16G16B16_SINT;
                    if (type == NumericType::Float) return VK_FORMAT_R16G16B16_SFLOAT;
                }
                if (bits == BitDepth::B32) {
                    if (type == NumericType::Uint) return VK_FORMAT_R32G32B32_UINT;
                    if (type == NumericType::Sint) return VK_FORMAT_R32G32B32_SINT;
                    if (type == NumericType::Float) return VK_FORMAT_R32G32B32_SFLOAT;
                }
            }

            // BGR Family
            if (channels == ChannelOrder::BGR) {
                if (bits == BitDepth::B8) {
                    if (type == NumericType::Unorm) return VK_FORMAT_B8G8R8_UNORM;
                    if (type == NumericType::Snorm) return VK_FORMAT_B8G8R8_SNORM;
                    if (type == NumericType::Uint) return VK_FORMAT_B8G8R8_UINT;
                    if (type == NumericType::Sint) return VK_FORMAT_B8G8R8_SINT;
                    if (type == NumericType::Srgb) return VK_FORMAT_B8G8R8_SRGB;
                }
            }

            // RGBA Family
            if (channels == ChannelOrder::RGBA) {
                if (bits == BitDepth::B8) {
                    if (type == NumericType::Unorm) return VK_FORMAT_R8G8B8A8_UNORM;
                    if (type == NumericType::Snorm) return VK_FORMAT_R8G8B8A8_SNORM;
                    if (type == NumericType::Uint) return VK_FORMAT_R8G8B8A8_UINT;
                    if (type == NumericType::Sint) return VK_FORMAT_R8G8B8A8_SINT;
                    if (type == NumericType::Srgb) return VK_FORMAT_R8G8B8A8_SRGB;
                }
                if (bits == BitDepth::B16) {
                    if (type == NumericType::Unorm) return VK_FORMAT_R16G16B16A16_UNORM;
                    if (type == NumericType::Snorm) return VK_FORMAT_R16G16B16A16_SNORM;
                    if (type == NumericType::Uint) return VK_FORMAT_R16G16B16A16_UINT;
                    if (type == NumericType::Sint) return VK_FORMAT_R16G16B16A16_SINT;
                    if (type == NumericType::Float) return VK_FORMAT_R16G16B16A16_SFLOAT;
                }
                if (bits == BitDepth::B32) {
                    if (type == NumericType::Uint) return VK_FORMAT_R32G32B32A32_UINT;
                    if (type == NumericType::Sint) return VK_FORMAT_R32G32B32A32_SINT;
                    if (type == NumericType::Float) return VK_FORMAT_R32G32B32A32_SFLOAT;
                }
            }

            // BGRA Family
            if (channels == ChannelOrder::BGRA) {
                if (bits == BitDepth::B8) {
                    if (type == NumericType::Unorm) return VK_FORMAT_B8G8R8A8_UNORM;
                    if (type == NumericType::Snorm) return VK_FORMAT_B8G8R8A8_SNORM;
                    if (type == NumericType::Uint) return VK_FORMAT_B8G8R8A8_UINT;
                    if (type == NumericType::Sint) return VK_FORMAT_B8G8R8A8_SINT;
                    if (type == NumericType::Srgb) return VK_FORMAT_B8G8R8A8_SRGB;
                }
            }

            // Depth & Depth-Stencil
            if (channels == ChannelOrder::D) {
                if (bits == BitDepth::B16 && type == NumericType::Unorm) return VK_FORMAT_D16_UNORM;
                if (bits == BitDepth::B32 && type == NumericType::Float) return VK_FORMAT_D32_SFLOAT;
            }

            if (channels == ChannelOrder::DS) {
                if (bits == BitDepth::B16 && type == NumericType::Uint) return VK_FORMAT_D16_UNORM_S8_UINT;
                if (bits == BitDepth::B24 && type == NumericType::Uint) return VK_FORMAT_D24_UNORM_S8_UINT;
                if (bits == BitDepth::B32 && type == NumericType::Float) return VK_FORMAT_D32_SFLOAT_S8_UINT;
            }

            return VK_FORMAT_UNDEFINED;
        }

        // Accessors
        [[nodiscard]] constexpr ChannelOrder getChannelOrder() const { return channels; }
        [[nodiscard]] constexpr BitDepth getBitDepth() const { return bits; }
        [[nodiscard]] constexpr NumericType getNumericType() const { return type; }

        // Comparison Operators
        constexpr bool operator==(const Format &other) const {
            return native() == other.native();
        }

        constexpr bool operator!=(const Format &other) const {
            return !(*this == other);
        }

        constexpr bool operator==(VkFormat rawFormat) const {
            return native() == rawFormat;
        }

        constexpr bool operator!=(VkFormat rawFormat) const {
            return native() != rawFormat;
        }

    private:
        ChannelOrder channels = ChannelOrder::Undefined;
        BitDepth bits = BitDepth::Undefined;
        NumericType type = NumericType::Undefined;
    };

    enum class ResultCode {
        Success = VK_SUCCESS,
        Suboptimal = VK_SUBOPTIMAL_KHR,
        OutOfDate = VK_ERROR_OUT_OF_DATE_KHR,
        Timeout = VK_TIMEOUT,
        NotReady = VK_NOT_READY,
        ErrorDeviceLost = VK_ERROR_DEVICE_LOST,
        ErrorOutOfMemory = VK_ERROR_OUT_OF_DATE_KHR,
        UnknownError = VK_ERROR_UNKNOWN,
    };

    struct Result {
        VkResult value;
        ResultCode code;

        // Constructors
        Result(VkResult res)
            : value(res), code(static_cast<ResultCode>(res)) {
        }

        Result(ResultCode resCode)
            : value(static_cast<VkResult>(resCode)), code(resCode) {
        }

        // Evaluates to true if the result represents SUCCESS or SUBOPTIMAL
        explicit operator bool() const {
            return code == ResultCode::Success || code == ResultCode::Suboptimal;
        }

        // Comparison operators for VkResult
        bool operator==(VkResult res) const { return value == res; }
        bool operator!=(VkResult res) const { return value != res; }

        // Comparison operators for custom ResultCode enum
        bool operator==(ResultCode resCode) const { return code == resCode; }
        bool operator!=(ResultCode resCode) const { return code != resCode; }

        // Implicit conversion back to VkResult for native Vulkan API calls
        operator VkResult() const { return value; }

        // Explicit conversion to LavaVK ResultCode
        operator ResultCode() const { return code; }
    };
} // namespace LavaVK

#endif // LAVAVK_INSTANCE_H
