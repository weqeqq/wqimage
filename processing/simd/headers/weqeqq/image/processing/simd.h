#pragma once

#include <cstddef>
#include <cstdint>

namespace weqeqq::image::simd {

enum class Color {
  kRgb,
  kRgba,
  kGrayscale,
  kCmyk,
  kCount,
};

enum class Blending {
  kNormal,
  kDissolve,

  kDarken,
  kMultiply,
  kColorBurn,
  kLinearBurn,
  kDarkerColor,

  kLighten,
  kScreen,
  kColorDodge,
  kLinearDodge,
  kLighterColor,

  kOverlay,
  kSoftLight,
  kHardLight,
  kVividLight,
  kLinearLight,
  kPinLight,
  kHardMix,

  kDifference,
  kExclusion,
  kSubtract,
  kDivide,

  kHue,
  kSaturation,
  kColor,
  kLuminosity,

  kCount,
};

enum class Alpha {
  kStraight,
  kPremultiplied,

  kCount,
};

inline constexpr std::size_t ChannelCount(Color color) noexcept {
  switch (color) {
    case Color::kRgb:
      return 3;
    case Color::kRgba:
      return 4;
    case Color::kGrayscale:
      return 1;
    case Color::kCmyk:
      return 4;
    default:
      return 0;
  }
}

void PremultiplySpan(std::uint8_t* data, std::size_t pixel_count) noexcept;

void UnpremultiplySpan(std::uint8_t* data, std::size_t pixel_count) noexcept;

void BlendRow(Color color, Blending blending, Alpha alpha, std::uint8_t* dst,
              const std::uint8_t* src, std::size_t count, std::size_t dx_start,
              std::size_t dy, std::uint8_t opacity) noexcept;

}  // namespace weqeqq::image::simd
