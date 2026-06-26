module;

#include <weqeqq/image/processing/simd.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>

export module weqeqq.image.processing:blending;

import :error;
export import weqeqq.image;
export import weqeqq.parallel;

export namespace weqeqq::image {

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

inline constexpr int kBlendOpacityMin = 0;
inline constexpr int kBlendOpacityMax = 100;

}  // namespace weqeqq::image

namespace weqeqq::image {

inline constexpr bool kDebug =
#ifdef NDEBUG
    false;
#else
    true;
#endif

namespace internal {

struct Region {
  std::size_t sx_start = 0;
  std::size_t sy_start = 0;
  std::size_t dx_start = 0;
  std::size_t dy_start = 0;
  std::size_t w        = 0;
  std::size_t h        = 0;

  [[nodiscard]] bool Empty() const { return w == 0 || h == 0; }

  static Region Compute(std::size_t dw, std::size_t dh, std::size_t sw,
                        std::size_t sh, std::ptrdiff_t x_off,
                        std::ptrdiff_t y_off) {
    auto dx = std::max<std::ptrdiff_t>(0, x_off);
    auto dy = std::max<std::ptrdiff_t>(0, y_off);
    auto sx = std::max<std::ptrdiff_t>(0, -x_off);
    auto sy = std::max<std::ptrdiff_t>(0, -y_off);

    if (dx >= static_cast<std::ptrdiff_t>(dw) ||
        dy >= static_cast<std::ptrdiff_t>(dh) ||
        sx >= static_cast<std::ptrdiff_t>(sw) ||
        sy >= static_cast<std::ptrdiff_t>(sh)) {
      return {};
    }

    Region r;
    r.dx_start = static_cast<std::size_t>(dx);
    r.dy_start = static_cast<std::size_t>(dy);
    r.sx_start = static_cast<std::size_t>(sx);
    r.sy_start = static_cast<std::size_t>(sy);
    r.w        = std::min(dw - r.dx_start, sw - r.sx_start);
    r.h        = std::min(dh - r.dy_start, sh - r.sy_start);
    return r;
  }
};

[[noreturn]] void ThrowBlendOpacityOutOfRange(int value) {
  throw error::TypedErrorBuilder(ProcessingError::kBlendOpacityOutOfRange,
                                 "blend opacity is out of range")
      .Field("value", value)
      .Field("min", kBlendOpacityMin)
      .Field("max", kBlendOpacityMax)
      .Details("Blend opacity accepts only values inside the supported range.")
      .Hint("Pass an opacity between 0 and 100 inclusive.")
      .Build();
}

void ValidateBlendOpacity(int opacity) {
  if (opacity < kBlendOpacityMin || opacity > kBlendOpacityMax) {
    ThrowBlendOpacityOutOfRange(opacity);
  }
}

inline std::uint8_t PercentToOpacityByte(int opacity) noexcept {
  return static_cast<std::uint8_t>((opacity * 255 + 50) / 100);
}

void BlendInPlaceImpl(Buffer& dst, const Buffer& src, std::ptrdiff_t x_off,
                      std::ptrdiff_t y_off, Blending blending, Alpha alpha,
                      int opacity, parallel::ExecutionPolicy exec) {
  ValidateBlendOpacity(opacity);
  if (opacity == kBlendOpacityMin) {
    return;
  }

  const auto color = dst.Color();

  if (color == Color::kCmyk) {
    auto dst_rgb = ConvertColor(dst, Color::kRgb);
    auto src_rgb = ConvertColor(src, Color::kRgb);
    BlendInPlaceImpl(dst_rgb, src_rgb, x_off, y_off, blending, alpha, opacity,
                     exec);
    ConvertColor(dst_rgb, dst);
    return;
  }

  const auto region = Region::Compute(dst.Width(), dst.Height(), src.Width(),
                                      src.Height(), x_off, y_off);
  if (region.Empty()) {
    return;
  }

  const auto opacity_u8 = PercentToOpacityByte(opacity);
  const auto channels   = dst.ChannelCount();
  const auto dst_stride = dst.StrideBytes();
  const auto src_stride = src.StrideBytes();
  auto* dst_data        = dst.Data();
  const auto* src_data  = src.Data();

  const auto simd_color = static_cast<simd::Color>(static_cast<int>(color));
  const auto simd_blending =
      static_cast<simd::Blending>(static_cast<int>(blending));
  const auto simd_alpha = static_cast<simd::Alpha>(static_cast<int>(alpha));

  parallel::ForEachIndex(
      exec, std::ptrdiff_t{0}, static_cast<std::ptrdiff_t>(region.h),
      [&](std::ptrdiff_t row) {
        const auto dy = region.dy_start + static_cast<std::size_t>(row);
        const auto sy = region.sy_start + static_cast<std::size_t>(row);
        auto* dst_row = dst_data + dy * dst_stride + region.dx_start * channels;
        const auto* src_row =
            src_data + sy * src_stride + region.sx_start * channels;
        simd::BlendRow(simd_color, simd_blending, simd_alpha, dst_row, src_row,
                       region.w, region.dx_start, dy, opacity_u8);
      });
}

void PremultiplyChunked(std::uint8_t* data, std::size_t pixel_count,
                        parallel::ExecutionPolicy execution, bool premultiply) {
  if (pixel_count == 0) {
    return;
  }

  constexpr std::size_t kChunk = 8192;
  const auto chunk_count       = (pixel_count + kChunk - 1) / kChunk;

  parallel::ForEachIndex(
      execution, std::ptrdiff_t{0},
      static_cast<std::ptrdiff_t>(chunk_count), [&](std::ptrdiff_t chunk) {
        const auto start = static_cast<std::size_t>(chunk) * kChunk;
        const auto count = std::min(kChunk, pixel_count - start);
        auto* span       = data + start * 4;
        if (premultiply) {
          simd::PremultiplySpan(span, count);
        } else {
          simd::UnpremultiplySpan(span, count);
        }
      });
}

}  // namespace internal

export void PremultiplyInPlace(
    Buffer& buffer, parallel::ExecutionPolicy execution =
                        parallel::Execution::kSequential) noexcept(!kDebug) {
  internal::PremultiplyChunked(buffer.Data(), buffer.PixelCount(), execution,
                               true);
}

export [[nodiscard]] Buffer Premultiply(
    Buffer buffer, parallel::ExecutionPolicy execution =
                       parallel::Execution::kSequential) noexcept(!kDebug) {
  PremultiplyInPlace(buffer, execution);
  return buffer;
}

export void UnpremultiplyInPlace(
    Buffer& buffer, parallel::ExecutionPolicy execution =
                        parallel::Execution::kSequential) noexcept(!kDebug) {
  internal::PremultiplyChunked(buffer.Data(), buffer.PixelCount(), execution,
                               false);
}

export [[nodiscard]] Buffer Unpremultiply(
    Buffer buffer, parallel::ExecutionPolicy execution =
                       parallel::Execution::kSequential) noexcept(!kDebug) {
  UnpremultiplyInPlace(buffer, execution);
  return buffer;
}

export void BlendInPlace(
    Buffer& destination, const Buffer& source, std::ptrdiff_t x_offset,
    std::ptrdiff_t y_offset, Blending blending = Blending::kNormal,
    Alpha alpha = Alpha::kStraight, int opacity = kBlendOpacityMax,
    parallel::ExecutionPolicy execution = parallel::Execution::kSequential) {
  internal::BlendInPlaceImpl(destination, source, x_offset, y_offset, blending,
                             alpha, opacity, execution);
}

export void BlendInPlace(
    Buffer& destination, const Buffer& source,
    Blending blending = Blending::kNormal, Alpha alpha = Alpha::kStraight,
    int opacity                         = kBlendOpacityMax,
    parallel::ExecutionPolicy execution = parallel::Execution::kSequential) {
  BlendInPlace(destination, source, 0, 0, blending, alpha, opacity, execution);
}

export [[nodiscard]] Buffer Blend(
    Buffer destination, const Buffer& source, std::ptrdiff_t x_offset,
    std::ptrdiff_t y_offset, Blending blending = Blending::kNormal,
    Alpha alpha = Alpha::kStraight, int opacity = kBlendOpacityMax,
    parallel::ExecutionPolicy execution = parallel::Execution::kSequential) {
  BlendInPlace(destination, source, x_offset, y_offset, blending, alpha,
               opacity, execution);
  return destination;
}

export [[nodiscard]] Buffer Blend(
    Buffer destination, const Buffer& source,
    Blending blending = Blending::kNormal, Alpha alpha = Alpha::kStraight,
    int opacity                         = kBlendOpacityMax,
    parallel::ExecutionPolicy execution = parallel::Execution::kSequential) {
  return Blend(std::move(destination), source, 0, 0, blending, alpha, opacity,
               execution);
}

}  // namespace weqeqq::image
