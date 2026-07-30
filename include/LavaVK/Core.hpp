#ifndef LAVAVK_CORE_HPP
#define LAVAVK_CORE_HPP
#include <vulkan/vulkan_core.h>

namespace LavaVK {
    /**
     * @brief Ordering and count of color/depth channels making up a pixel format.
     * @details Used together with #BitDepth and #NumericType by #Format to
     * describe a `VkFormat` in a structured, queryable way instead of a
     * single opaque enum value.
     */
    enum class ChannelOrder {
        R, /**< Single red channel. */
        RG, /**< Red, green channels. */
        RGB, /**< Red, green, blue channels. */
        BGR, /**< Blue, green, red channels. */
        RGBA, /**< Red, green, blue, alpha channels. */
        BGRA, /**< Blue, green, red, alpha channels. */
        D, /**< Depth channel only. */
        DS, /**< Depth and stencil channels. */
        Undefined /**< No channel layout / format is undefined. */
    };

    /**
     * @brief Number of bits per channel making up a pixel format.
     * @details Used together with #ChannelOrder and #NumericType by #Format.
     */
    enum class BitDepth {
        B8, /**< 8 bits per channel. */
        B16, /**< 16 bits per channel. */
        B24, /**< 24 bits per channel (used by some depth/stencil formats). */
        B32, /**< 32 bits per channel. */
        Undefined /**< No bit depth / format is undefined. */
    };

    /**
     * @brief Numeric interpretation applied to the raw bits of each channel.
     * @details Used together with #ChannelOrder and #BitDepth by #Format.
     */
    enum class NumericType {
        Unorm, /**< Unsigned normalized: integer values mapped to [0, 1]. */
        Snorm, /**< Signed normalized: integer values mapped to [-1, 1]. */
        Uint, /**< Unsigned integer, used directly. */
        Sint, /**< Signed integer, used directly. */
        Float, /**< Signed floating point. */
        Uscaled, /**< Unsigned integer, scaled to float without normalization. */
        Sscaled, /**< Signed integer, scaled to float without normalization. */
        Srgb, /**< Unsigned normalized with an sRGB (nonlinear) transfer function. */
        Undefined /**< No numeric interpretation / format is undefined. */
    };

    /**
     * @brief Structured, human-readable representation of a Vulkan pixel format.
     *
     * @details
     * Raw `VkFormat` values are a single flat enum (e.g.
     * `VK_FORMAT_R8G8B8A8_SRGB`) that encodes channel order, bit depth, and
     * numeric type all at once, which makes them awkward to construct or
     * inspect programmatically. `Format` instead stores those three
     * properties (#ChannelOrder, #BitDepth, #NumericType) separately and
     * converts to/from `VkFormat` on demand via #native() and the implicit
     * `VkFormat` conversion operator, so a `LavaVK::Format` can be passed
     * anywhere a `VkFormat` is expected while still being easy to build and
     * compare.
     *
     * Example:
     * @code
     * // Build a format instead of hard-coding VK_FORMAT_R32G32B32_SFLOAT
     * LavaVK::Format positionFormat(
     *     LavaVK::ChannelOrder::RGB,
     *     LavaVK::BitDepth::B32,
     *     LavaVK::NumericType::Float);
     *
     * // Wrap an existing VkFormat and inspect it
     * LavaVK::Format surfaceFormat(VK_FORMAT_B8G8R8A8_SRGB);
     * if (surfaceFormat.getNumericType() == LavaVK::NumericType::Srgb) { ... }
     *
     * // Implicitly converts back to VkFormat when needed
     * VkFormat raw = positionFormat;
     * @endcode
     */
    class Format {
    public:
        /**
         * @brief Default constructor. Creates a format equivalent to `VK_FORMAT_UNDEFINED`.
         */
        constexpr Format()
            : channels(ChannelOrder::Undefined), bits(BitDepth::Undefined), type(NumericType::Undefined) {
        }

        /**
         * @brief Constructs a format from its three structured components.
         * @param channels Channel order (e.g. RGBA).
         * @param bits Bit depth per channel.
         * @param type Numeric interpretation of each channel's bits.
         */
        constexpr Format(ChannelOrder channels, BitDepth bits, NumericType type)
            : channels(channels), bits(bits), type(type) {
        }

        /**
         * @brief Constructs a format directly from a raw `VkFormat`.
         * @details Converts the Vulkan enum value back into the structured
         * #ChannelOrder / #BitDepth / #NumericType representation. Formats
         * with no equivalent mapping resolve to #ChannelOrder::Undefined,
         * #BitDepth::Undefined, and #NumericType::Undefined.
         * @param format The native `VkFormat` to wrap.
         */
        constexpr Format(VkFormat format) {
            *this = format;
        }

        /**
         * @brief Implicit conversion to the native `VkFormat` enum.
         * @details Allows a `LavaVK::Format` to be passed directly anywhere
         * a `VkFormat` is expected, without an explicit call to #native().
         * @return The `VkFormat` value equivalent to this format's components.
         */
        constexpr operator VkFormat() const {
            return native();
        }

        /**
         * @brief Copy-assigns this format's components from another `Format`.
         * @param other The format to copy from.
         * @return Reference to this format.
         */
        constexpr Format &operator=(const Format &other) = default;

        /**
         * @brief Assigns this format's components by decoding a raw `VkFormat`.
         * @details Performs the same conversion as the `VkFormat` constructor,
         * replacing this object's #ChannelOrder, #BitDepth, and #NumericType
         * in place. Unrecognized formats reset all three to `Undefined`.
         * @param format The native `VkFormat` to decode and assign from.
         * @return Reference to this format.
         */
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
         * @brief Checks if the format represents an undefined state (`VK_FORMAT_UNDEFINED`).
         * @return `true` if this format's components have no valid `VkFormat` mapping
         * (equivalently, if #native() returns `VK_FORMAT_UNDEFINED`); `false` otherwise.
         */
        [[nodiscard]] constexpr bool isUndefined() const {
            return native() == VK_FORMAT_UNDEFINED;
        }

        /**
         * @brief Evaluates the corresponding native `VkFormat` enum value.
         * @details Encodes this format's #ChannelOrder, #BitDepth, and
         * #NumericType back into the single flat `VkFormat` enum Vulkan
         * expects, via a lookup that mirrors the reverse mapping performed
         * by the `VkFormat` assignment operator.
         * @return The matching `VkFormat` value, or `VK_FORMAT_UNDEFINED` if
         * this combination of components has no Vulkan equivalent.
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

        /**
         * @brief Returns this format's channel order.
         * @return The #ChannelOrder component (e.g. RGBA, BGRA, D).
         */
        [[nodiscard]] constexpr ChannelOrder getChannelOrder() const { return channels; }

        /**
         * @brief Returns this format's per-channel bit depth.
         * @return The #BitDepth component (e.g. B8, B32).
         */
        [[nodiscard]] constexpr BitDepth getBitDepth() const { return bits; }

        /**
         * @brief Returns this format's numeric interpretation.
         * @return The #NumericType component (e.g. Unorm, Float, Srgb).
         */
        [[nodiscard]] constexpr NumericType getNumericType() const { return type; }

        /**
         * @brief Compares two formats for equality.
         * @param other The format to compare against.
         * @return `true` if both formats resolve to the same `VkFormat`; `false` otherwise.
         */
        constexpr bool operator==(const Format &other) const {
            return native() == other.native();
        }

        /**
         * @brief Compares two formats for inequality.
         * @param other The format to compare against.
         * @return `true` if the formats resolve to different `VkFormat` values; `false` otherwise.
         */
        constexpr bool operator!=(const Format &other) const {
            return !(*this == other);
        }

        /**
         * @brief Compares this format against a raw `VkFormat` for equality.
         * @param rawFormat The native Vulkan format to compare against.
         * @return `true` if this format resolves to @p rawFormat; `false` otherwise.
         */
        constexpr bool operator==(VkFormat rawFormat) const {
            return native() == rawFormat;
        }

        /**
         * @brief Compares this format against a raw `VkFormat` for inequality.
         * @param rawFormat The native Vulkan format to compare against.
         * @return `true` if this format does not resolve to @p rawFormat; `false` otherwise.
         */
        constexpr bool operator!=(VkFormat rawFormat) const {
            return native() != rawFormat;
        }

    private:
        ChannelOrder channels = ChannelOrder::Undefined;
        BitDepth bits = BitDepth::Undefined;
        NumericType type = NumericType::Undefined;
    };

    /**
     * @brief Named subset of `VkResult` codes LavaVK callers commonly need to branch on.
     * @details Not exhaustive: values without a dedicated entry still round-trip
     * correctly through #Result (via its raw `VkResult`), but are only
     * distinguishable through #Result::value rather than through this enum.
     *
     * @warning #ErrorOutOfMemory is currently mapped to the same underlying
     * value as #OutOfDate (`VK_ERROR_OUT_OF_DATE_KHR`) rather than
     * `VK_ERROR_OUT_OF_HOST_MEMORY`/`VK_ERROR_OUT_OF_DEVICE_MEMORY`; treat
     * the two as indistinguishable via #ResultCode until this is corrected.
     */
    enum class ResultCode {
        Success = VK_SUCCESS, /**< The operation completed successfully. */
        Suboptimal = VK_SUBOPTIMAL_KHR, /**< A swapchain no longer matches the surface exactly but can still be used. */
        OutOfDate = VK_ERROR_OUT_OF_DATE_KHR, /**< A swapchain is incompatible with the surface and must be recreated. */
        Timeout = VK_TIMEOUT, /**< A wait operation did not complete within the given timeout. */
        NotReady = VK_NOT_READY, /**< A fence or query is not yet signaled/available. */
        ErrorDeviceLost = VK_ERROR_DEVICE_LOST, /**< The logical or physical device was lost. */
        ErrorOutOfMemory = VK_ERROR_OUT_OF_DATE_KHR, /**< Reserved for out-of-memory errors. See warning above. */
        UnknownError = VK_ERROR_UNKNOWN, /**< An unrecognized/unmapped error occurred. */
    };

    /**
     * @brief Lightweight wrapper around `VkResult` with LavaVK-friendly comparisons and truthiness.
     *
     * @details
     * Many Vulkan calls (`vkAcquireNextImageKHR`, `vkQueuePresentKHR`,
     * `vkWaitForFences`, ...) return a `VkResult` that is not simply
     * success/failure — values like `VK_SUBOPTIMAL_KHR` or
     * `VK_ERROR_OUT_OF_DATE_KHR` are meaningful non-error conditions the
     * caller must react to. `Result` stores both the raw `VkResult` and a
     * best-effort #ResultCode classification, and can be evaluated directly
     * in a boolean context (`true` for #ResultCode::Success or
     * #ResultCode::Suboptimal) so common call sites read naturally, while
     * `value` and `code` remain available for finer-grained handling.
     *
     * Example:
     * @code
     * uint32_t imageIndex = 0;
     * LavaVK::Result acquireResult = swapchain.acquireImage(imageIndex);
     * if (!acquireResult) {
     *     swapchain.recreate();
     *     continue;
     * }
     * @endcode
     */
    struct Result {
        /** @brief The raw `VkResult` value returned by the originating Vulkan call. */
        VkResult value;
        /** @brief Best-effort classification of #value as a #ResultCode. */
        ResultCode code;

        /**
         * @brief Constructs a Result from a raw `VkResult`.
         * @param res The native Vulkan result code to wrap.
         */
        Result(VkResult res)
            : value(res), code(static_cast<ResultCode>(res)) {
        }

        /**
         * @brief Constructs a Result from a LavaVK #ResultCode.
         * @param resCode The result code to wrap; also stored as the equivalent raw `VkResult`.
         */
        Result(ResultCode resCode)
            : value(static_cast<VkResult>(resCode)), code(resCode) {
        }

        /**
         * @brief Evaluates the result in a boolean context.
         * @return `true` if #code is #ResultCode::Success or #ResultCode::Suboptimal
         * (i.e. the operation's output can still be used); `false` otherwise.
         */
        explicit operator bool() const {
            return code == ResultCode::Success || code == ResultCode::Suboptimal;
        }

        /**
         * @brief Compares this result's raw value against a `VkResult`.
         * @param res The native result code to compare against.
         * @return `true` if #value equals @p res.
         */
        bool operator==(VkResult res) const { return value == res; }

        /**
         * @brief Compares this result's raw value against a `VkResult`.
         * @param res The native result code to compare against.
         * @return `true` if #value does not equal @p res.
         */
        bool operator!=(VkResult res) const { return value != res; }

        /**
         * @brief Compares this result's classification against a #ResultCode.
         * @param resCode The result code to compare against.
         * @return `true` if #code equals @p resCode.
         */
        bool operator==(ResultCode resCode) const { return code == resCode; }

        /**
         * @brief Compares this result's classification against a #ResultCode.
         * @param resCode The result code to compare against.
         * @return `true` if #code does not equal @p resCode.
         */
        bool operator!=(ResultCode resCode) const { return code != resCode; }

        /**
         * @brief Implicit conversion back to the native `VkResult`, for passing to Vulkan APIs.
         * @return The raw #value.
         */
        operator VkResult() const { return value; }

        /**
         * @brief Explicit conversion to the LavaVK #ResultCode classification.
         * @return The #code this result was classified as.
         */
        operator ResultCode() const { return code; }
    };
}

#endif //LAVAVK_CORE_HPP