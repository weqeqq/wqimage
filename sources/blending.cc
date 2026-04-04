

#include <weqeqq/image/blending.h>
#include <weqeqq/image/color_conversion.h>

#include <algorithm>
#include <array>
#include <cassert>

#ifndef __CLANGD__
#include "weqeqq/image/config.h"
#endif

#if WQIMAGE_SIMD

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "blending.cc"

#include <hwy/foreach_target.h>
#include <hwy/highway.h>

HWY_BEFORE_NAMESPACE();

#endif

namespace weqeqq::image::internal {
#if WQIMAGE_SIMD
namespace HWY_NAMESPACE {

namespace hn = hwy::HWY_NAMESPACE;
#endif

// ═══════════════════════════════════════════════════════
//                        Region
// ═══════════════════════════════════════════════════════

struct Region {
  std::size_t sx_start = 0;
  std::size_t sy_start = 0;
  std::size_t dx_start = 0;
  std::size_t dy_start = 0;
  std::size_t w        = 0;
  std::size_t h        = 0;

  bool Empty() const { return w == 0 || h == 0; }

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
        sy >= static_cast<std::ptrdiff_t>(sh))
      return {};

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

// ═══════════════════════════════════════════════════════
//                      Helpers
// ═══════════════════════════════════════════════════════

inline constexpr auto kRgbaChannelCount = 4;

inline uint16_t Div255(uint16_t v) noexcept {
  v += 128;
  return (v + (v >> 8)) >> 8;
}

inline uint32_t Div255Wide(uint32_t v) noexcept {
  v += 128;
  return (v + (v >> 8)) >> 8;
}

#if WQIMAGE_SIMD

template <typename D, HWY_IF_U16_D(D)>
HWY_INLINE auto Div255(D d, hn::VFromD<D> v) noexcept {
  v = hn::Add(v, hn::Set(d, uint16_t{128}));
  return hn::ShiftRight<8>(hn::Add(v, hn::ShiftRight<8>(v)));
}

template <typename BlockFn, typename TailFn>
inline void ForEachSimdBlocksAndScalarTail(std::size_t item_count,
                                           std::size_t lane_count,
                                           BlockFn &&block_fn,
                                           TailFn &&tail_fn) {
  const std::size_t block_count = item_count / lane_count;
  const std::size_t tail_start  = block_count * lane_count;
  block_fn(block_count);
  tail_fn(tail_start, item_count - tail_start);
}

#endif

// ═══════════════════════════════════════════════════════
//                    Premultiply
// ═══════════════════════════════════════════════════════

inline void PremultiplyPixelScalar(std::uint8_t &r, std::uint8_t &g,
                                   std::uint8_t &b, std::uint8_t a) noexcept {
  r = Div255(r * a);
  g = Div255(g * a);
  b = Div255(b * a);
}

#if WQIMAGE_SIMD

template <typename D, HWY_IF_U16_D(D)>
HWY_INLINE void PremultiplyPixelSimd(D d, hn::VFromD<D> &r, hn::VFromD<D> &g,
                                     hn::VFromD<D> &b,
                                     hn::VFromD<D> a) noexcept {
  r = Div255(d, hn::Mul(r, a));
  g = Div255(d, hn::Mul(g, a));
  b = Div255(d, hn::Mul(b, a));
}

#endif  // WQIMAGE_SIMD

inline void PremultiplyInPlaceImplScalar(
    std::uint8_t *data, std::size_t pixel_count,
    parallel::ExecutionPolicy execution) noexcept {
  if (pixel_count == 0) {
    return;
  }
  parallel::ForEachIndex(execution, 0, pixel_count, [&](std::size_t index) {
    const std::size_t offset = index * kRgbaChannelCount;
    PremultiplyPixelScalar(data[offset + 0], data[offset + 1], data[offset + 2],
                           data[offset + 3]);
  });
}

#if WQIMAGE_SIMD

void PremultiplyInPlaceImplSimd(std::uint8_t *data, std::size_t pixel_count,
                                parallel::ExecutionPolicy execution) noexcept {
  const auto du16 = hn::ScalableTag<std::uint16_t>{};
  const auto du8h = hn::Rebind<std::uint8_t, decltype(du16)>{};

  const std::size_t lane_count = hn::Lanes(du16);
  ForEachSimdBlocksAndScalarTail(
      pixel_count, lane_count,
      [&](std::size_t block_count) {
        parallel::ForEachIndex(
            execution, 0, block_count, [&](std::size_t block_index) {
              const std::size_t offset =
                  block_index * lane_count * kRgbaChannelCount;

              auto r = hn::Undefined(du8h), g = hn::Undefined(du8h),
                   b = hn::Undefined(du8h), a = hn::Undefined(du8h);

              hn::LoadInterleaved4(du8h, data + offset, r, g, b, a);

              auto r_u16 = hn::PromoteTo(du16, r),
                   g_u16 = hn::PromoteTo(du16, g),
                   b_u16 = hn::PromoteTo(du16, b),
                   a_u16 = hn::PromoteTo(du16, a);

              PremultiplyPixelSimd(du16, r_u16, g_u16, b_u16, a_u16);

              r = hn::DemoteTo(du8h, r_u16);
              g = hn::DemoteTo(du8h, g_u16);
              b = hn::DemoteTo(du8h, b_u16);
              a = hn::DemoteTo(du8h, a_u16);

              hn::StoreInterleaved4(r, g, b, a, du8h, data + offset);
            });
      },
      [&](std::size_t tail_start, std::size_t tail_count) {
        PremultiplyInPlaceImplScalar(data + tail_start * kRgbaChannelCount,
                                     tail_count,
                                     parallel::Execution::kSequential);
      });
}

#endif

inline void PremultiplyInPlace(Buffer &buffer,
                               parallel::ExecutionPolicy execution) {
#if WQIMAGE_SIMD
  PremultiplyInPlaceImplSimd(buffer.Data(), buffer.PixelCount(), execution);
#else
  PremultiplyInPlaceImplScalar(buffer.Data(), buffer.PixelCount(), execution);
#endif
}

// ═══════════════════════════════════════════════════════
//                   Unpremultiply
// ═══════════════════════════════════════════════════════

inline constexpr auto kRecipTable = [] consteval {
  std::array<std::uint16_t, 256> table{};
  table[0] = 0;
  for (int alpha = 1; alpha < 256; alpha++) {
    table[alpha] = static_cast<std::uint16_t>((255 * 256 + alpha - 1) / alpha);
  }
  return table;
}();

inline void UnpremultiplyPixelScalar(std::uint8_t &r, std::uint8_t &g,
                                     std::uint8_t &b, std::uint8_t a) {
  if (a == 0) {
    r = g = b = 0;
  } else {
    const auto mul = kRecipTable[a];
    r = static_cast<std::uint8_t>(std::min<std::uint16_t>(255, (r * mul) >> 8));
    g = static_cast<std::uint8_t>(std::min<std::uint16_t>(255, (g * mul) >> 8));
    b = static_cast<std::uint8_t>(std::min<std::uint16_t>(255, (b * mul) >> 8));
  }
}

#if WQIMAGE_SIMD
template <typename D, typename V = hn::VFromD<D>, HWY_IF_U16_D(D)>
HWY_INLINE void UnpremultiplyPixelSimd(D d, V &r, V &g, V &b, V a) noexcept {
  const auto lane_count = hn::Lanes(d);
  const auto v255       = hn::Set(d, uint16_t{255});
  const auto zero       = hn::Zero(d);

  HWY_ALIGN std::uint16_t alpha_buffer[hn::MaxLanes(d)];
  HWY_ALIGN std::uint16_t mul_buffer[hn::MaxLanes(d)];

  hn::Store(a, d, alpha_buffer);
  for (std::size_t index = 0; index < lane_count; index++) {
    mul_buffer[index] = kRecipTable[alpha_buffer[index]];
  }
  auto mul  = hn::Load(d, mul_buffer);
  auto mask = hn::Eq(a, zero);

  r = hn::IfThenZeroElse(mask,
                         hn::Min(hn::ShiftRight<8>(hn::Mul(r, mul)), v255));
  g = hn::IfThenZeroElse(mask,
                         hn::Min(hn::ShiftRight<8>(hn::Mul(g, mul)), v255));
  b = hn::IfThenZeroElse(mask,
                         hn::Min(hn::ShiftRight<8>(hn::Mul(b, mul)), v255));
}
#endif  // WQIMAGE_SIMD

void UnpremultiplyInPlaceImplScalar(
    std::uint8_t *data, std::size_t pixel_count,
    parallel::ExecutionPolicy execution) noexcept {
  if (pixel_count == 0) {
    return;
  }
  parallel::ForEachIndex(execution, 0, pixel_count, [&](std::size_t index) {
    const std::size_t offset = index * kRgbaChannelCount;
    UnpremultiplyPixelScalar(data[offset + 0], data[offset + 1],
                             data[offset + 2], data[offset + 3]);
  });
}

#if WQIMAGE_SIMD

void UnpremultiplyInPlaceImplSimd(
    std::uint8_t *data, std::size_t pixel_count,
    parallel::ExecutionPolicy execution) noexcept {
  const auto du16 = hn::ScalableTag<std::uint16_t>{};
  const auto du8h = hn::Rebind<std::uint8_t, decltype(du16)>{};

  const std::size_t lane_count = hn::Lanes(du16);
  ForEachSimdBlocksAndScalarTail(
      pixel_count, lane_count,
      [&](std::size_t block_count) {
        parallel::ForEachIndex(
            execution, 0, block_count, [&](std::size_t block_index) {
              const std::size_t offset =
                  block_index * lane_count * kRgbaChannelCount;

              auto r = hn::Undefined(du8h), g = hn::Undefined(du8h),
                   b = hn::Undefined(du8h), a = hn::Undefined(du8h);

              hn::LoadInterleaved4(du8h, data + offset, r, g, b, a);

              auto r_u16 = hn::PromoteTo(du16, r),
                   g_u16 = hn::PromoteTo(du16, g),
                   b_u16 = hn::PromoteTo(du16, b),
                   a_u16 = hn::PromoteTo(du16, a);

              UnpremultiplyPixelSimd(du16, r_u16, g_u16, b_u16, a_u16);

              r = hn::DemoteTo(du8h, r_u16);
              g = hn::DemoteTo(du8h, g_u16);
              b = hn::DemoteTo(du8h, b_u16);
              a = hn::DemoteTo(du8h, a_u16);

              hn::StoreInterleaved4(r, g, b, a, du8h, data + offset);
            });
      },
      [&](std::size_t tail_start, std::size_t tail_count) {
        UnpremultiplyInPlaceImplScalar(data + tail_start * kRgbaChannelCount,
                                       tail_count,
                                       parallel::Execution::kSequential);
      });
}
#endif  // WQIMAGE_SIMD

inline void UnpremultiplyInPlace(Buffer &buffer,
                                 parallel::ExecutionPolicy execution) {
#if WQIMAGE_SIMD
  UnpremultiplyInPlaceImplSimd(buffer.Data(), buffer.PixelCount(), execution);
#else
  UnpremultiplyInPlaceImplScalar(buffer.Data(), buffer.PixelCount(), execution);
#endif
}

// ═══════════════════════════════════════════════════════
//            BlendCategory and BlendChannel
// ═══════════════════════════════════════════════════════

// --- Category tagging -------------------------------------------------------

enum class BlendCategory { kSeparable, kComponent, kSpecial };

template <Blending B>
inline constexpr BlendCategory CategoryOf = BlendCategory::kSeparable;

template <>
inline constexpr auto CategoryOf<Blending::kDarkerColor> =  //
    BlendCategory::kComponent;

template <>
inline constexpr auto CategoryOf<Blending::kLighterColor> =  //
    BlendCategory::kComponent;

template <>
inline constexpr auto CategoryOf<Blending::kHue> =  //
    BlendCategory::kComponent;

template <>
inline constexpr auto CategoryOf<Blending::kSaturation> =  //
    BlendCategory::kComponent;

template <>
inline constexpr auto CategoryOf<Blending::kColor> =  //
    BlendCategory::kComponent;

template <>
inline constexpr auto CategoryOf<Blending::kLuminosity> =  //
    BlendCategory::kComponent;

template <>
inline constexpr auto CategoryOf<Blending::kDissolve> =  //
    BlendCategory::kSpecial;

// --- BlendChannel: scalar implementations ----------------------------------

template <Blending Blending_>
  requires(Blending_ == Blending::kNormal)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  destination = source;
}

template <Blending Blending_>
  requires(Blending_ == Blending::kDarken)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  destination = std::min(destination, source);
}

template <Blending Blending_>
  requires(Blending_ == Blending::kMultiply)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  destination = Div255(destination * source);
}

template <Blending Blending_>
  requires(Blending_ == Blending::kColorBurn)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  auto d = destination, s = source;
  if (s == 0) {
    destination = 0;
  } else {
    auto v      = (255 - d) * 255 / s;
    destination = static_cast<std::uint8_t>(v > 255 ? 0 : 255 - v);
  }
}

template <Blending Blending_>
  requires(Blending_ == Blending::kLinearBurn)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  auto sum    = static_cast<int>(destination) + source;
  destination = static_cast<std::uint8_t>(sum > 255 ? sum - 255 : 0);
}

template <Blending Blending_>
  requires(Blending_ == Blending::kLighten)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  destination = std::max(destination, source);
}

template <Blending Blending_>
  requires(Blending_ == Blending::kScreen)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  destination = static_cast<std::uint8_t>(destination + source -
                                          Div255(destination * source));
}

template <Blending Blending_>
  requires(Blending_ == Blending::kColorDodge)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  if (source == 255) {
    destination = 255;
  } else {
    auto v      = static_cast<int>(destination) * 255 / (255 - source);
    destination = static_cast<std::uint8_t>(std::min(v, 255));
  }
}

template <Blending Blending_>
  requires(Blending_ == Blending::kLinearDodge)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  auto sum    = static_cast<int>(destination) + source;
  destination = static_cast<std::uint8_t>(std::min(sum, 255));
}

template <Blending Blending_>
  requires(Blending_ == Blending::kOverlay)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  auto d = destination, s = source;
  if (2 * d < 255) {
    destination = static_cast<std::uint8_t>(Div255(2 * d * s));
  } else {
    destination =
        static_cast<std::uint8_t>(255 - Div255(2 * (255 - d) * (255 - s)));
  }
}

inline constexpr auto kSqrt255Table = [] consteval {
  std::array<std::uint8_t, 256> table{};
  table[0] = 0;
  for (int i = 1; i < 256; i++) {
    double v = i / 255.0;
    double s = 0.0;
    double x = v;
    for (int j = 0; j < 30; j++) {
      x = 0.5 * (x + v / x);
    }
    s        = x;
    table[i] = static_cast<std::uint8_t>(s * 255.0);
  }
  return table;
}();

template <Blending Blending_>
  requires(Blending_ == Blending::kSoftLight)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  int d = destination, s = source;
  int result;
  if (2 * s <= 255) {
    result = d - (255 - 2 * s) * d * (255 - d) / (255 * 255);
  } else if (4 * d <= 255) {
    int D  = ((16 * d - 12 * 255) * d + 4 * 255 * 255) * d / (255 * 255);
    result = d + (2 * s - 255) * (D - d) / 255;
  } else {
    int sq = kSqrt255Table[d];
    result = d + (2 * s - 255) * (sq - d) / 255;
  }
  destination = static_cast<std::uint8_t>(std::clamp(result, 0, 255));
}

template <Blending Blending_>
  requires(Blending_ == Blending::kHardLight)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  auto d = destination, s = source;
  if (2 * s < 255) {
    destination = static_cast<std::uint8_t>(Div255(2 * d * s));
  } else {
    destination =
        static_cast<std::uint8_t>(255 - Div255(2 * (255 - d) * (255 - s)));
  }
}

template <Blending Blending_>
  requires(Blending_ == Blending::kVividLight)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  auto d = destination, s = source;
  if (s == 0) {
    destination = 0;
  } else if (2 * s <= 255) {
    auto s2     = static_cast<std::uint8_t>(2 * s);
    auto v      = (255 - d) * 255 / s2;
    destination = static_cast<std::uint8_t>(v > 255 ? 0 : 255 - v);
  } else if (s == 255) {
    destination = 255;
  } else {
    auto s2     = 2 * s - 255;
    auto v      = static_cast<int>(d) * 255 / (255 - s2);
    destination = static_cast<std::uint8_t>(std::min(v, 255));
  }
}

template <Blending Blending_>
  requires(Blending_ == Blending::kLinearLight)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  auto v      = static_cast<int>(destination) + 2 * source - 255;
  destination = static_cast<std::uint8_t>(std::clamp(v, 0, 255));
}

template <Blending Blending_>
  requires(Blending_ == Blending::kPinLight)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  auto d = destination, s = source;
  if (2 * s <= 255) {
    destination = std::min(d, static_cast<std::uint8_t>(2 * s));
  } else {
    destination = std::max(d, static_cast<std::uint8_t>(2 * s - 255));
  }
}

template <Blending Blending_>
  requires(Blending_ == Blending::kHardMix)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  destination = static_cast<std::uint8_t>(
      static_cast<int>(destination) + source >= 255 ? 255 : 0);
}

template <Blending Blending_>
  requires(Blending_ == Blending::kDifference)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  destination = static_cast<std::uint8_t>(
      destination > source ? destination - source : source - destination);
}

template <Blending Blending_>
  requires(Blending_ == Blending::kExclusion)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  destination = static_cast<std::uint8_t>(
      destination + source - Div255Wide(2u * destination * source));
}

template <Blending Blending_>
  requires(Blending_ == Blending::kSubtract)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  destination = static_cast<std::uint8_t>(
      destination > source ? destination - source : 0);
}

template <Blending Blending_>
  requires(Blending_ == Blending::kDivide)
void BlendChannelScalar(std::uint8_t &destination, std::uint8_t &source) {
  if (source == 0) {
    destination = 255;
  } else {
    auto v      = static_cast<int>(destination) * 255 / source;
    destination = static_cast<std::uint8_t>(std::min(v, 255));
  }
}

#if WQIMAGE_SIMD

// --- BlendChannel: SIMD helpers --------------------------------------------

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
HWY_INLINE void BlendChannelSimdViaScalarLanes(D d, V &destination, V source) {
  const auto lane_count = hn::Lanes(d);
  HWY_ALIGN std::uint16_t d_buf[hn::MaxLanes(d)];
  HWY_ALIGN std::uint16_t s_buf[hn::MaxLanes(d)];
  hn::Store(destination, d, d_buf);
  hn::Store(source, d, s_buf);
  for (std::size_t i = 0; i < lane_count; i++) {
    auto dv = static_cast<std::uint8_t>(d_buf[i]);
    auto sv = static_cast<std::uint8_t>(s_buf[i]);
    BlendChannelScalar<Blending_>(dv, sv);
    d_buf[i] = dv;
  }
  destination = hn::Load(d, d_buf);
}

// --- BlendChannel: SIMD implementations ------------------------------------

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kNormal)
HWY_INLINE void BlendChannelSimd(D, V &destination, V source) {
  destination = source;
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kDarken)
HWY_INLINE void BlendChannelSimd(D, V &destination, V source) {
  destination = hn::Min(destination, source);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kMultiply)
HWY_INLINE void BlendChannelSimd(D d, V &destination, V source) {
  destination = Div255(d, hn::Mul(destination, source));
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kColorBurn)
HWY_INLINE void BlendChannelSimd(D d, V &destination, V source) {
  BlendChannelSimdViaScalarLanes<Blending::kColorBurn>(d, destination, source);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kLinearBurn)
HWY_INLINE void BlendChannelSimd(D d, V &destination, V source) {
  const auto v255 = hn::Set(d, uint16_t{255});
  auto sum        = hn::Add(destination, source);
  auto underflow  = hn::Lt(sum, v255);
  destination     = hn::IfThenZeroElse(underflow, hn::Sub(sum, v255));
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kLighten)
HWY_INLINE void BlendChannelSimd(D, V &destination, V source) {
  destination = hn::Max(destination, source);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kScreen)
HWY_INLINE void BlendChannelSimd(D d, V &destination, V source) {
  destination = hn::Sub(hn::Add(destination, source),
                        Div255(d, hn::Mul(destination, source)));
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kColorDodge)
HWY_INLINE void BlendChannelSimd(D d, V &destination, V source) {
  BlendChannelSimdViaScalarLanes<Blending::kColorDodge>(d, destination, source);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kLinearDodge)
HWY_INLINE void BlendChannelSimd(D d, V &destination, V source) {
  const auto v255 = hn::Set(d, uint16_t{255});
  destination     = hn::Min(v255, hn::Add(destination, source));
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kOverlay)
HWY_INLINE void BlendChannelSimd(D d, V &destination, V source) {
  const auto v255 = hn::Set(d, uint16_t{255});
  const auto two  = hn::Set(d, uint16_t{2});
  auto d2         = hn::Mul(two, destination);
  auto mask       = hn::Lt(d2, v255);
  auto lo         = Div255(d, hn::Mul(d2, source));
  auto inv_d      = hn::Sub(v255, destination);
  auto inv_s      = hn::Sub(v255, source);
  auto hi     = hn::Sub(v255, Div255(d, hn::Mul(hn::Mul(two, inv_d), inv_s)));
  destination = hn::IfThenElse(mask, lo, hi);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kSoftLight)
HWY_INLINE void BlendChannelSimd(D d, V &destination, V source) {
  BlendChannelSimdViaScalarLanes<Blending::kSoftLight>(d, destination, source);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kHardLight)
HWY_INLINE void BlendChannelSimd(D d, V &destination, V source) {
  const auto v255 = hn::Set(d, uint16_t{255});
  const auto two  = hn::Set(d, uint16_t{2});
  auto s2         = hn::Mul(two, source);
  auto mask       = hn::Lt(s2, v255);
  auto lo         = Div255(d, hn::Mul(hn::Mul(two, destination), source));
  auto inv_d      = hn::Sub(v255, destination);
  auto inv_s      = hn::Sub(v255, source);
  auto hi     = hn::Sub(v255, Div255(d, hn::Mul(hn::Mul(two, inv_d), inv_s)));
  destination = hn::IfThenElse(mask, lo, hi);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kVividLight)
HWY_INLINE void BlendChannelSimd(D d, V &destination, V source) {
  BlendChannelSimdViaScalarLanes<Blending::kVividLight>(d, destination, source);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kLinearLight)
HWY_INLINE void BlendChannelSimd(D d, V &destination, V source) {
  const auto v255 = hn::Set(d, uint16_t{255});
  const auto zero = hn::Zero(d);
  const auto two  = hn::Set(d, uint16_t{2});
  // d + 2*s - 255, clamped to [0, 255]
  auto sum = hn::Add(destination, hn::Mul(two, source));
  auto raw = hn::Sub(sum, v255);
  // If sum < 255, result is 0; else min(raw, 255)
  auto underflow = hn::Lt(sum, v255);
  destination    = hn::IfThenElse(underflow, zero, hn::Min(v255, raw));
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kPinLight)
HWY_INLINE void BlendChannelSimd(D d, V &destination, V source) {
  const auto v255 = hn::Set(d, uint16_t{255});
  const auto two  = hn::Set(d, uint16_t{2});
  auto s2         = hn::Mul(two, source);
  auto mask       = hn::Le(s2, v255);
  auto lo         = hn::Min(destination, s2);
  auto hi         = hn::Max(destination, hn::Sub(s2, v255));
  destination     = hn::IfThenElse(mask, lo, hi);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kHardMix)
HWY_INLINE void BlendChannelSimd(D d, V &destination, V source) {
  const auto v255 = hn::Set(d, uint16_t{255});
  auto sum        = hn::Add(destination, source);
  auto mask       = hn::Ge(sum, v255);
  destination     = hn::IfThenElseZero(mask, v255);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kDifference)
HWY_INLINE void BlendChannelSimd(D, V &destination, V source) {
  auto gt_mask = hn::Gt(destination, source);
  auto hi      = hn::Sub(destination, source);
  auto lo      = hn::Sub(source, destination);
  destination  = hn::IfThenElse(gt_mask, hi, lo);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kExclusion)
HWY_INLINE void BlendChannelSimd(D d, V &destination, V source) {
  BlendChannelSimdViaScalarLanes<Blending::kExclusion>(d, destination, source);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kSubtract)
HWY_INLINE void BlendChannelSimd(D, V &destination, V source) {
  auto mask   = hn::Gt(destination, source);
  destination = hn::IfThenElseZero(mask, hn::Sub(destination, source));
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>>
  requires(Blending_ == Blending::kDivide)
HWY_INLINE void BlendChannelSimd(D d, V &destination, V source) {
  BlendChannelSimdViaScalarLanes<Blending::kDivide>(d, destination, source);
}

#endif  // WQIMAGE_SIMD

// ═══════════════════════════════════════════════════════
//                       BlendColor
// ═══════════════════════════════════════════════════════

// --- BlendColor: scalar helper math (HSL/int domain) -----------------------

inline std::uint8_t LuminosityScalar(std::uint8_t r, std::uint8_t g,
                                     std::uint8_t b) noexcept {
  return static_cast<std::uint8_t>((77 * r + 150 * g + 29 * b) >> 8);
}

inline int LumI(int r, int g, int b) noexcept {
  return (77 * r + 150 * g + 29 * b) >> 8;
}

inline void ClipColor(int &r, int &g, int &b) noexcept {
  const int lum = LumI(r, g, b);
  const int mn  = std::min({r, g, b});
  const int mx  = std::max({r, g, b});
  if (mn < 0) {
    r = lum + ((r - lum) * lum) / (lum - mn);
    g = lum + ((g - lum) * lum) / (lum - mn);
    b = lum + ((b - lum) * lum) / (lum - mn);
  }
  if (mx > 255) {
    r = lum + ((r - lum) * (255 - lum)) / (mx - lum);
    g = lum + ((g - lum) * (255 - lum)) / (mx - lum);
    b = lum + ((b - lum) * (255 - lum)) / (mx - lum);
  }
}

inline void SetLum(int &r, int &g, int &b, int lum) noexcept {
  const int d = lum - LumI(r, g, b);
  r += d;
  g += d;
  b += d;
  ClipColor(r, g, b);
}

inline int SatI(int r, int g, int b) noexcept {
  return std::max({r, g, b}) - std::min({r, g, b});
}

inline void SetSatInPlace(int &cmin, int &cmid, int &cmax, int sat) noexcept {
  if (cmax > cmin) {
    cmid = ((cmid - cmin) * sat) / (cmax - cmin);
    cmax = sat;
  } else {
    cmid = cmax = 0;
  }
  cmin = 0;
}

inline void SetSat(int &r, int &g, int &b, int sat) noexcept {
  int *ptr[3] = {&r, &g, &b};
  if (*ptr[0] > *ptr[1]) std::swap(ptr[0], ptr[1]);
  if (*ptr[1] > *ptr[2]) std::swap(ptr[1], ptr[2]);
  if (*ptr[0] > *ptr[1]) std::swap(ptr[0], ptr[1]);
  SetSatInPlace(*ptr[0], *ptr[1], *ptr[2], sat);
}

// --- BlendColor: scalar implementations ------------------------------------

template <Blending Blending_>
  requires(CategoryOf<Blending_> == BlendCategory::kComponent &&
           Blending_ == Blending::kLighterColor)
inline void BlendColorScalar(std::uint8_t &dr, std::uint8_t &dg,
                             std::uint8_t &db, std::uint8_t sr, std::uint8_t sg,
                             std::uint8_t sb) noexcept {
  if (LuminosityScalar(sr, sg, sb) > LuminosityScalar(dr, dg, db)) {
    dr = sr;
    dg = sg;
    db = sb;
  }
}

template <Blending Blending_>
  requires(CategoryOf<Blending_> == BlendCategory::kComponent &&
           Blending_ == Blending::kLighterColor)
inline void BlendColorScalar(std::uint8_t &dy, std::uint8_t sy) noexcept {
  BlendChannelScalar<Blending::kLighten>(dy, sy);
}

template <Blending Blending_>
  requires(CategoryOf<Blending_> == BlendCategory::kComponent &&
           Blending_ == Blending::kDarkerColor)
inline void BlendColorScalar(std::uint8_t &dr, std::uint8_t &dg,
                             std::uint8_t &db, std::uint8_t sr, std::uint8_t sg,
                             std::uint8_t sb) noexcept {
  if (LuminosityScalar(sr, sg, sb) < LuminosityScalar(dr, dg, db)) {
    dr = sr;
    dg = sg;
    db = sb;
  }
}

template <Blending Blending_>
  requires(CategoryOf<Blending_> == BlendCategory::kComponent &&
           Blending_ == Blending::kDarkerColor)
inline void BlendColorScalar(std::uint8_t &dy, std::uint8_t sy) noexcept {
  BlendChannelScalar<Blending::kDarken>(dy, sy);
}

template <Blending Blending_>
  requires(Blending_ == Blending::kHue)
inline void BlendColorScalar(std::uint8_t &dr, std::uint8_t &dg,
                             std::uint8_t &db, std::uint8_t sr, std::uint8_t sg,
                             std::uint8_t sb) noexcept {
  int r = sr, g = sg, b = sb;
  SetSat(r, g, b, SatI(dr, dg, db));
  SetLum(r, g, b, LumI(dr, dg, db));
  dr = static_cast<std::uint8_t>(std::clamp(r, 0, 255));
  dg = static_cast<std::uint8_t>(std::clamp(g, 0, 255));
  db = static_cast<std::uint8_t>(std::clamp(b, 0, 255));
}

template <Blending Blending_>
  requires(Blending_ == Blending::kHue)
inline void BlendColorScalar(std::uint8_t & /*dy*/,
                             std::uint8_t /*sy*/) noexcept {
  // Grayscale has no hue, destination unchanged
}

template <Blending Blending_>
  requires(Blending_ == Blending::kSaturation)
inline void BlendColorScalar(std::uint8_t &dr, std::uint8_t &dg,
                             std::uint8_t &db, std::uint8_t sr, std::uint8_t sg,
                             std::uint8_t sb) noexcept {
  int r = dr, g = dg, b = db;
  SetSat(r, g, b, SatI(sr, sg, sb));
  SetLum(r, g, b, LumI(dr, dg, db));
  dr = static_cast<std::uint8_t>(std::clamp(r, 0, 255));
  dg = static_cast<std::uint8_t>(std::clamp(g, 0, 255));
  db = static_cast<std::uint8_t>(std::clamp(b, 0, 255));
}

template <Blending Blending_>
  requires(Blending_ == Blending::kSaturation)
inline void BlendColorScalar(std::uint8_t & /*dy*/,
                             std::uint8_t /*sy*/) noexcept {
  // Grayscale has no saturation, destination unchanged
}

template <Blending Blending_>
  requires(Blending_ == Blending::kColor)
inline void BlendColorScalar(std::uint8_t &dr, std::uint8_t &dg,
                             std::uint8_t &db, std::uint8_t sr, std::uint8_t sg,
                             std::uint8_t sb) noexcept {
  int r = sr, g = sg, b = sb;
  SetLum(r, g, b, LumI(dr, dg, db));
  dr = static_cast<std::uint8_t>(std::clamp(r, 0, 255));
  dg = static_cast<std::uint8_t>(std::clamp(g, 0, 255));
  db = static_cast<std::uint8_t>(std::clamp(b, 0, 255));
}

template <Blending Blending_>
  requires(Blending_ == Blending::kColor)
inline void BlendColorScalar(std::uint8_t & /*dy*/,
                             std::uint8_t /*sy*/) noexcept {
  // Grayscale: no hue/sat to transfer, destination unchanged
}

template <Blending Blending_>
  requires(Blending_ == Blending::kLuminosity)
inline void BlendColorScalar(std::uint8_t &dr, std::uint8_t &dg,
                             std::uint8_t &db, std::uint8_t sr, std::uint8_t sg,
                             std::uint8_t sb) noexcept {
  int r = dr, g = dg, b = db;
  SetLum(r, g, b, LumI(sr, sg, sb));
  dr = static_cast<std::uint8_t>(std::clamp(r, 0, 255));
  dg = static_cast<std::uint8_t>(std::clamp(g, 0, 255));
  db = static_cast<std::uint8_t>(std::clamp(b, 0, 255));
}

template <Blending Blending_>
  requires(Blending_ == Blending::kLuminosity)
inline void BlendColorScalar(std::uint8_t &dy, std::uint8_t sy) noexcept {
  dy = sy;
}

#if WQIMAGE_SIMD

// --- BlendColor: SIMD helpers/implementations ------------------------------

template <typename D, HWY_IF_U16_D(D)>
HWY_INLINE hn::VFromD<D> LuminositySimd(D d, hn::VFromD<D> r, hn::VFromD<D> g,
                                        hn::VFromD<D> b) {
  auto rm = hn::Mul(r, hn::Set(d, uint16_t{77}));
  auto gm = hn::Mul(g, hn::Set(d, uint16_t{150}));
  auto bm = hn::Mul(b, hn::Set(d, uint16_t{29}));

  return hn::ShiftRight<8>(hn::Add(hn::Add(rm, gm), bm));
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>,
          HWY_IF_U16_D(D)>
  requires(CategoryOf<Blending_> == BlendCategory::kComponent &&
           Blending_ == Blending::kLighterColor)
HWY_INLINE void BlendColorSimd(D d, V &dr, V &dg, V &db, V sr, V sg, V sb) {
  auto mask =
      hn::Gt(LuminositySimd(d, sr, sg, sb), LuminositySimd(d, dr, dg, db));
  dr = hn::IfThenElse(mask, sr, dr);
  dg = hn::IfThenElse(mask, sg, dg);
  db = hn::IfThenElse(mask, sb, db);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>,
          HWY_IF_U16_D(D)>
  requires(CategoryOf<Blending_> == BlendCategory::kComponent &&
           Blending_ == Blending::kLighterColor)
HWY_INLINE void BlendColorSimd(D d, V &dy, V sy) {
  BlendChannelSimd<Blending::kLighten>(d, dy, sy);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>,
          HWY_IF_U16_D(D)>
  requires(CategoryOf<Blending_> == BlendCategory::kComponent &&
           Blending_ == Blending::kDarkerColor)
HWY_INLINE void BlendColorSimd(D d, V &dr, V &dg, V &db, V sr, V sg, V sb) {
  auto mask =
      hn::Lt(LuminositySimd(d, sr, sg, sb), LuminositySimd(d, dr, dg, db));
  dr = hn::IfThenElse(mask, sr, dr);
  dg = hn::IfThenElse(mask, sg, dg);
  db = hn::IfThenElse(mask, sb, db);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>,
          HWY_IF_U16_D(D)>
  requires(CategoryOf<Blending_> == BlendCategory::kComponent &&
           Blending_ == Blending::kDarkerColor)
HWY_INLINE void BlendColorSimd(D d, V &dy, V sy) {
  BlendChannelSimd<Blending::kDarken>(d, dy, sy);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>,
          HWY_IF_U16_D(D)>
HWY_INLINE void BlendColorSimdViaScalarLanesRgb(D d, V &dr, V &dg, V &db, V sr,
                                                V sg, V sb) {
  const auto lane_count = hn::Lanes(d);
  HWY_ALIGN std::uint16_t dr_b[hn::MaxLanes(d)], dg_b[hn::MaxLanes(d)],
      db_b[hn::MaxLanes(d)];
  HWY_ALIGN std::uint16_t sr_b[hn::MaxLanes(d)], sg_b[hn::MaxLanes(d)],
      sb_b[hn::MaxLanes(d)];
  hn::Store(dr, d, dr_b);
  hn::Store(dg, d, dg_b);
  hn::Store(db, d, db_b);
  hn::Store(sr, d, sr_b);
  hn::Store(sg, d, sg_b);
  hn::Store(sb, d, sb_b);
  for (std::size_t i = 0; i < lane_count; i++) {
    auto dr_i = static_cast<std::uint8_t>(dr_b[i]);
    auto dg_i = static_cast<std::uint8_t>(dg_b[i]);
    auto db_i = static_cast<std::uint8_t>(db_b[i]);
    auto sr_i = static_cast<std::uint8_t>(sr_b[i]);
    auto sg_i = static_cast<std::uint8_t>(sg_b[i]);
    auto sb_i = static_cast<std::uint8_t>(sb_b[i]);
    BlendColorScalar<Blending_>(dr_i, dg_i, db_i, sr_i, sg_i, sb_i);
    dr_b[i] = dr_i;
    dg_b[i] = dg_i;
    db_b[i] = db_i;
  }
  dr = hn::Load(d, dr_b);
  dg = hn::Load(d, dg_b);
  db = hn::Load(d, db_b);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>,
          HWY_IF_U16_D(D)>
HWY_INLINE void BlendColorSimdViaScalarLanesGray(D d, V &dy, V sy) {
  const auto lane_count = hn::Lanes(d);
  HWY_ALIGN std::uint16_t dy_b[hn::MaxLanes(d)], sy_b[hn::MaxLanes(d)];
  hn::Store(dy, d, dy_b);
  hn::Store(sy, d, sy_b);
  for (std::size_t i = 0; i < lane_count; i++) {
    auto dy_i = static_cast<std::uint8_t>(dy_b[i]);
    auto sy_i = static_cast<std::uint8_t>(sy_b[i]);
    BlendColorScalar<Blending_>(dy_i, sy_i);
    dy_b[i] = dy_i;
  }
  dy = hn::Load(d, dy_b);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>,
          HWY_IF_U16_D(D)>
  requires(Blending_ == Blending::kHue || Blending_ == Blending::kSaturation ||
           Blending_ == Blending::kColor || Blending_ == Blending::kLuminosity)
HWY_INLINE void BlendColorSimd(D d, V &dr, V &dg, V &db, V sr, V sg, V sb) {
  BlendColorSimdViaScalarLanesRgb<Blending_>(d, dr, dg, db, sr, sg, sb);
}

template <Blending Blending_, typename D, typename V = hn::VFromD<D>,
          HWY_IF_U16_D(D)>
  requires(Blending_ == Blending::kHue || Blending_ == Blending::kSaturation ||
           Blending_ == Blending::kColor || Blending_ == Blending::kLuminosity)
HWY_INLINE void BlendColorSimd(D d, V &dy, V sy) {
  BlendColorSimdViaScalarLanesGray<Blending_>(d, dy, sy);
}

#endif  // WQIMAGE_SIMD

// ═══════════════════════════════════════════════════════
//                    AlphaComposite
// ═══════════════════════════════════════════════════════

template <Alpha Alpha_>
inline void AlphaCompositePixelScalar(std::uint8_t &dr, std::uint8_t &dg,
                                      std::uint8_t &db, std::uint8_t &da,
                                      std::uint8_t sr, std::uint8_t sg,
                                      std::uint8_t sb,
                                      std::uint8_t sa) noexcept {
  if constexpr (Alpha_ == Alpha::kPremultiplied) {
    const auto inv = 255 - sa;
    dr             = static_cast<uint8_t>(sr + Div255(dr * inv));
    dg             = static_cast<uint8_t>(sg + Div255(dg * inv));
    db             = static_cast<uint8_t>(sb + Div255(db * inv));
    da = static_cast<uint8_t>(std::min<uint16_t>(255, sa + Div255(da * inv)));
  } else {
    PremultiplyPixelScalar(sr, sg, sb, sa);
    PremultiplyPixelScalar(dr, dg, db, da);
    AlphaCompositePixelScalar<Alpha::kPremultiplied>(dr, dg, db, da, sr, sg, sb,
                                                     sa);
    UnpremultiplyPixelScalar(dr, dg, db, da);
  }
}

#if WQIMAGE_SIMD

template <Alpha Alpha_, typename D, typename V = hn::VFromD<D>, HWY_IF_U16_D(D)>
HWY_INLINE void AlphaCompositePixelSimd(D d, V &dr, V &dg, V &db, V &da, V sr,
                                        V sg, V sb, V sa) {
  const auto v255 = hn::Set(d, uint16_t{255});
  const auto inv  = hn::Sub(v255, sa);

  if constexpr (Alpha_ == Alpha::kPremultiplied) {
    dr = hn::Add(sr, Div255(d, hn::Mul(dr, inv)));
    dg = hn::Add(sg, Div255(d, hn::Mul(dg, inv)));
    db = hn::Add(sb, Div255(d, hn::Mul(db, inv)));
    da = hn::Min(v255, hn::Add(sa, Div255(d, hn::Mul(da, inv))));
  } else {
    PremultiplyPixelSimd(d, sr, sg, sb, sa);
    PremultiplyPixelSimd(d, dr, dg, db, da);
    AlphaCompositePixelSimd<Alpha::kPremultiplied>(d, dr, dg, db, da, sr, sg,
                                                   sb, sa);
    UnpremultiplyPixelSimd(d, dr, dg, db, da);
  }
}
#endif  // WQIMAGE_SIMD

// ═══════════════════════════════════════════════════════
//                    Blend Kernels
// ═══════════════════════════════════════════════════════

// --- Blend Kernels: shared constants and traits ----------------------------

inline constexpr auto kRgbChannelCount       = 3;
inline constexpr auto kGrayscaleChannelCount = 1;
inline constexpr std::uint16_t kU8Max        = 255;

template <Color C>
struct ColorTraits;

template <>
struct ColorTraits<Color::kRgba> {
  static constexpr std::size_t kChannelCount = kRgbaChannelCount;
  static constexpr bool HasAlpha             = true;
};

template <>
struct ColorTraits<Color::kRgb> {
  static constexpr std::size_t kChannelCount = kRgbChannelCount;
  static constexpr bool HasAlpha             = false;
};

template <>
struct ColorTraits<Color::kGrayscale> {
  static constexpr std::size_t kChannelCount = kGrayscaleChannelCount;
  static constexpr bool HasAlpha             = false;
};

template <Color C>
inline std::size_t PixelOffset(std::size_t stride, std::size_t x,
                               std::size_t y) noexcept {
  return y * stride + x * ColorTraits<C>::kChannelCount;
}

// --- Blend Kernels: traversal helpers --------------------------------------

template <typename RowFn>
inline void ForEachRegionRow(const Region &region,
                             parallel::ExecutionPolicy exec, RowFn &&row_fn) {
  parallel::ForEachIndex(
      exec, 0, static_cast<std::ptrdiff_t>(region.h), [&](std::ptrdiff_t row) {
        const auto dy = region.dy_start + static_cast<std::size_t>(row);
        const auto sy = region.sy_start + static_cast<std::size_t>(row);
        row_fn(dy, sy);
      });
}

// --- Blend Kernels: blend-dispatch helpers ---------------------------------

template <Blending B>
inline void ApplyBlendToRgbScalar(std::uint8_t &dr, std::uint8_t &dg,
                                  std::uint8_t &db, std::uint8_t sr,
                                  std::uint8_t sg, std::uint8_t sb) noexcept {
  if constexpr (CategoryOf<B> == BlendCategory::kSeparable) {
    BlendChannelScalar<B>(dr, sr);
    BlendChannelScalar<B>(dg, sg);
    BlendChannelScalar<B>(db, sb);
  } else if constexpr (CategoryOf<B> == BlendCategory::kComponent) {
    BlendColorScalar<B>(dr, dg, db, sr, sg, sb);
  }
}

template <Blending B>
inline void ApplyBlendToGrayScalar(std::uint8_t &dv, std::uint8_t sv) noexcept {
  if constexpr (CategoryOf<B> == BlendCategory::kSeparable) {
    BlendChannelScalar<B>(dv, sv);
  } else if constexpr (CategoryOf<B> == BlendCategory::kComponent) {
    BlendColorScalar<B>(dv, sv);
  }
}

#if WQIMAGE_SIMD
template <Blending B, typename D, typename V = hn::VFromD<D>, HWY_IF_U16_D(D)>
HWY_INLINE void ApplyBlendToRgbSimd(D d, V &dr, V &dg, V &db, V sr, V sg,
                                    V sb) {
  if constexpr (CategoryOf<B> == BlendCategory::kSeparable) {
    BlendChannelSimd<B>(d, dr, sr);
    BlendChannelSimd<B>(d, dg, sg);
    BlendChannelSimd<B>(d, db, sb);
  } else if constexpr (CategoryOf<B> == BlendCategory::kComponent) {
    BlendColorSimd<B>(d, dr, dg, db, sr, sg, sb);
  }
}

template <Blending B, typename D, typename V = hn::VFromD<D>, HWY_IF_U16_D(D)>
HWY_INLINE void ApplyBlendToGraySimd(D d, V &dv, V sv) {
  if constexpr (CategoryOf<B> == BlendCategory::kSeparable) {
    BlendChannelSimd<B>(d, dv, sv);
  } else if constexpr (CategoryOf<B> == BlendCategory::kComponent) {
    BlendColorSimd<B>(d, dv, sv);
  }
}
#endif  // WQIMAGE_SIMD

using BlendFn = void (*)(std::uint8_t *, std::size_t, const std::uint8_t *,
                         std::size_t, const Region &,
                         parallel::ExecutionPolicy) noexcept;

// --- Blend Kernels: generic scalar kernel ----------------------------------

template <Color C, Blending B, Alpha A>
void BlendInPlaceKernelScalar(std::uint8_t *dst_data, std::size_t dst_stride,
                              const std::uint8_t *src_data,
                              std::size_t src_stride, const Region &region,
                              parallel::ExecutionPolicy exec) noexcept {
  ForEachRegionRow(region, exec, [&](std::size_t dy, std::size_t sy) {
    for (std::size_t col = 0; col < region.w; col++) {
      const auto dx = region.dx_start + col;
      const auto sx = region.sx_start + col;

      if constexpr (C == Color::kRgba) {
        const auto d_off = PixelOffset<Color::kRgba>(dst_stride, dx, dy);
        const auto s_off = PixelOffset<Color::kRgba>(src_stride, sx, sy);

        auto dr = dst_data[d_off + 0], dg = dst_data[d_off + 1],
             db = dst_data[d_off + 2], da = dst_data[d_off + 3];
        auto sr = src_data[s_off + 0], sg = src_data[s_off + 1],
             sb = src_data[s_off + 2], sa = src_data[s_off + 3];

        auto br = dr, bg = dg, bb = db;
        ApplyBlendToRgbScalar<B>(br, bg, bb, sr, sg, sb);

        AlphaCompositePixelScalar<A>(dr, dg, db, da, br, bg, bb, sa);

        dst_data[d_off + 0] = dr;
        dst_data[d_off + 1] = dg;
        dst_data[d_off + 2] = db;
        dst_data[d_off + 3] = da;

      } else if constexpr (C == Color::kRgb) {
        const auto d_off = PixelOffset<Color::kRgb>(dst_stride, dx, dy);
        const auto s_off = PixelOffset<Color::kRgb>(src_stride, sx, sy);

        auto dr = dst_data[d_off + 0], dg = dst_data[d_off + 1],
             db = dst_data[d_off + 2];
        auto sr = src_data[s_off + 0], sg = src_data[s_off + 1],
             sb = src_data[s_off + 2];

        ApplyBlendToRgbScalar<B>(dr, dg, db, sr, sg, sb);

        dst_data[d_off + 0] = dr;
        dst_data[d_off + 1] = dg;
        dst_data[d_off + 2] = db;

      } else if constexpr (C == Color::kGrayscale) {
        const auto d_off = PixelOffset<Color::kGrayscale>(dst_stride, dx, dy);
        const auto s_off = PixelOffset<Color::kGrayscale>(src_stride, sx, sy);

        auto dy_val = dst_data[d_off];
        auto sy_val = src_data[s_off];

        ApplyBlendToGrayScalar<B>(dy_val, sy_val);
        dst_data[d_off] = dy_val;
      }
    }
  });
}

#if WQIMAGE_SIMD

// --- Blend Kernels: generic SIMD kernel ------------------------------------

template <Color C, Blending B, Alpha A>
void BlendInPlaceKernelSimd(std::uint8_t *dst_data, std::size_t dst_stride,
                            const std::uint8_t *src_data,
                            std::size_t src_stride, const Region &region,
                            parallel::ExecutionPolicy exec) noexcept {
  const auto du16              = hn::ScalableTag<std::uint16_t>{};
  const auto du8h              = hn::Rebind<std::uint8_t, decltype(du16)>{};
  const std::size_t lane_count = hn::Lanes(du16);

  ForEachRegionRow(region, exec, [&](std::size_t dy, std::size_t sy) {
    if constexpr (C == Color::kRgba) {
      const std::size_t block_count = region.w / lane_count;
      const std::size_t tail_start  = block_count * lane_count;

      for (std::size_t block = 0; block < block_count; block++) {
        const auto dx    = region.dx_start + block * lane_count;
        const auto sx    = region.sx_start + block * lane_count;
        const auto d_off = dy * dst_stride + dx * kRgbaChannelCount;
        const auto s_off = sy * src_stride + sx * kRgbaChannelCount;

        auto dr8 = hn::Undefined(du8h), dg8 = hn::Undefined(du8h),
             db8 = hn::Undefined(du8h), da8 = hn::Undefined(du8h);
        auto sr8 = hn::Undefined(du8h), sg8 = hn::Undefined(du8h),
             sb8 = hn::Undefined(du8h), sa8 = hn::Undefined(du8h);

        hn::LoadInterleaved4(du8h, dst_data + d_off, dr8, dg8, db8, da8);
        hn::LoadInterleaved4(du8h, src_data + s_off, sr8, sg8, sb8, sa8);

        auto dr = hn::PromoteTo(du16, dr8), dg = hn::PromoteTo(du16, dg8),
             db = hn::PromoteTo(du16, db8), da = hn::PromoteTo(du16, da8);
        auto sr = hn::PromoteTo(du16, sr8), sg = hn::PromoteTo(du16, sg8),
             sb = hn::PromoteTo(du16, sb8), sa = hn::PromoteTo(du16, sa8);

        auto br = dr, bg = dg, bb = db;
        ApplyBlendToRgbSimd<B>(du16, br, bg, bb, sr, sg, sb);
        AlphaCompositePixelSimd<A>(du16, dr, dg, db, da, br, bg, bb, sa);

        hn::StoreInterleaved4(hn::DemoteTo(du8h, dr), hn::DemoteTo(du8h, dg),
                              hn::DemoteTo(du8h, db), hn::DemoteTo(du8h, da),
                              du8h, dst_data + d_off);
      }

      for (std::size_t col = tail_start; col < region.w; col++) {
        const auto dx    = region.dx_start + col;
        const auto sx    = region.sx_start + col;
        const auto d_off = dy * dst_stride + dx * kRgbaChannelCount;
        const auto s_off = sy * src_stride + sx * kRgbaChannelCount;

        auto dr_c = dst_data[d_off + 0], dg_c = dst_data[d_off + 1],
             db_c = dst_data[d_off + 2], da_c = dst_data[d_off + 3];
        auto sr_c = src_data[s_off + 0], sg_c = src_data[s_off + 1],
             sb_c = src_data[s_off + 2], sa_c = src_data[s_off + 3];

        auto br_c = dr_c, bg_c = dg_c, bb_c = db_c;
        ApplyBlendToRgbScalar<B>(br_c, bg_c, bb_c, sr_c, sg_c, sb_c);
        AlphaCompositePixelScalar<A>(dr_c, dg_c, db_c, da_c, br_c, bg_c, bb_c,
                                     sa_c);

        dst_data[d_off + 0] = dr_c;
        dst_data[d_off + 1] = dg_c;
        dst_data[d_off + 2] = db_c;
        dst_data[d_off + 3] = da_c;
      }

    } else if constexpr (C == Color::kRgb) {
      const std::size_t block_count = region.w / lane_count;
      const std::size_t tail_start  = block_count * lane_count;

      for (std::size_t block = 0; block < block_count; block++) {
        const auto dx    = region.dx_start + block * lane_count;
        const auto sx    = region.sx_start + block * lane_count;
        const auto d_off = dy * dst_stride + dx * kRgbChannelCount;
        const auto s_off = sy * src_stride + sx * kRgbChannelCount;

        auto dr8 = hn::Undefined(du8h), dg8 = hn::Undefined(du8h),
             db8 = hn::Undefined(du8h);
        auto sr8 = hn::Undefined(du8h), sg8 = hn::Undefined(du8h),
             sb8 = hn::Undefined(du8h);

        hn::LoadInterleaved3(du8h, dst_data + d_off, dr8, dg8, db8);
        hn::LoadInterleaved3(du8h, src_data + s_off, sr8, sg8, sb8);

        auto dr = hn::PromoteTo(du16, dr8), dg = hn::PromoteTo(du16, dg8),
             db = hn::PromoteTo(du16, db8);
        auto sr = hn::PromoteTo(du16, sr8), sg = hn::PromoteTo(du16, sg8),
             sb = hn::PromoteTo(du16, sb8);

        ApplyBlendToRgbSimd<B>(du16, dr, dg, db, sr, sg, sb);

        hn::StoreInterleaved3(hn::DemoteTo(du8h, dr), hn::DemoteTo(du8h, dg),
                              hn::DemoteTo(du8h, db), du8h, dst_data + d_off);
      }

      for (std::size_t col = tail_start; col < region.w; col++) {
        const auto dx    = region.dx_start + col;
        const auto sx    = region.sx_start + col;
        const auto d_off = dy * dst_stride + dx * kRgbChannelCount;
        const auto s_off = sy * src_stride + sx * kRgbChannelCount;

        auto dr_c = dst_data[d_off + 0], dg_c = dst_data[d_off + 1],
             db_c = dst_data[d_off + 2];
        auto sr_c = src_data[s_off + 0], sg_c = src_data[s_off + 1],
             sb_c = src_data[s_off + 2];

        ApplyBlendToRgbScalar<B>(dr_c, dg_c, db_c, sr_c, sg_c, sb_c);

        dst_data[d_off + 0] = dr_c;
        dst_data[d_off + 1] = dg_c;
        dst_data[d_off + 2] = db_c;
      }

    } else if constexpr (C == Color::kGrayscale) {
      for (std::size_t col = 0; col < region.w; col++) {
        const auto dx    = region.dx_start + col;
        const auto sx    = region.sx_start + col;
        const auto d_off = dy * dst_stride + dx;
        const auto s_off = sy * src_stride + sx;

        auto dv = dst_data[d_off];
        auto sv = src_data[s_off];

        ApplyBlendToGrayScalar<B>(dv, sv);

        dst_data[d_off] = dv;
      }
    }
  });
}

#endif  // WQIMAGE_SIMD

// ═══════════════════════════════════════════════════════
//                  Dissolve Kernel
// ═══════════════════════════════════════════════════════

// --- Dissolve: deterministic noise helper ----------------------------------

inline std::uint32_t PixelHash(std::size_t x, std::size_t y) noexcept {
  auto v = static_cast<std::uint32_t>(x * 2654435761u ^ y * 2246822519u);
  v ^= v >> 16;
  v *= 0x45d9f3bu;
  v ^= v >> 16;
  return v;
}

// --- Dissolve: special-purpose kernel --------------------------------------

template <Color C, Alpha A>
void BlendInPlaceKernelDissolve(std::uint8_t *dst_data, std::size_t dst_stride,
                                const std::uint8_t *src_data,
                                std::size_t src_stride, const Region &region,
                                parallel::ExecutionPolicy exec) noexcept {
  ForEachRegionRow(region, exec, [&](std::size_t dy, std::size_t sy) {
    for (std::size_t col = 0; col < region.w; col++) {
      const auto dx = region.dx_start + col;
      const auto sx = region.sx_start + col;

      if constexpr (C == Color::kRgba) {
        const auto d_off = PixelOffset<Color::kRgba>(dst_stride, dx, dy);
        const auto s_off = PixelOffset<Color::kRgba>(src_stride, sx, sy);

        const auto sa = src_data[s_off + 3];
        // threshold in [0,255]: pixel appears if hash < sa
        if ((PixelHash(dx, dy) & 0xffu) < static_cast<std::uint32_t>(sa)) {
          dst_data[d_off + 0] = src_data[s_off + 0];
          dst_data[d_off + 1] = src_data[s_off + 1];
          dst_data[d_off + 2] = src_data[s_off + 2];
          dst_data[d_off + 3] = static_cast<std::uint8_t>(kU8Max);
        }

      } else if constexpr (C == Color::kRgb) {
        const auto d_off    = PixelOffset<Color::kRgb>(dst_stride, dx, dy);
        const auto s_off    = PixelOffset<Color::kRgb>(src_stride, sx, sy);
        dst_data[d_off + 0] = src_data[s_off + 0];
        dst_data[d_off + 1] = src_data[s_off + 1];
        dst_data[d_off + 2] = src_data[s_off + 2];

      } else if constexpr (C == Color::kGrayscale) {
        const auto d_off = PixelOffset<Color::kGrayscale>(dst_stride, dx, dy);
        const auto s_off = PixelOffset<Color::kGrayscale>(src_stride, sx, sy);
        dst_data[d_off]  = src_data[s_off];
      }
    }
  });
}

// ═══════════════════════════════════════════════════════
//                    Dispatch Table
// ═══════════════════════════════════════════════════════

// --- Dispatch: dimensions ---------------------------------------------------

inline constexpr auto kColorCount    = static_cast<int>(Color::kCount);
inline constexpr auto kBlendingCount = static_cast<int>(Blending::kCount);
inline constexpr auto kAlphaCount    = static_cast<int>(Alpha::kCount);

// --- Dispatch: function selection ------------------------------------------

template <Color C, Blending B, Alpha A>
constexpr BlendFn MakeBlendFn() {
  if constexpr (C == Color::kCmyk) {
    return nullptr;
  } else if constexpr (B == Blending::kDissolve) {
    return &BlendInPlaceKernelDissolve<C, A>;
  } else {
#if WQIMAGE_SIMD
    return &BlendInPlaceKernelSimd<C, B, A>;
#else
    return &BlendInPlaceKernelScalar<C, B, A>;
#endif
  }
}

// --- Dispatch: table construction ------------------------------------------

template <std::size_t... Is>
constexpr auto BuildDispatchTable(std::index_sequence<Is...>) {
  return std::array<BlendFn, sizeof...(Is)>{[] {
    constexpr auto i = Is;
    constexpr auto c = static_cast<Color>(i / (kBlendingCount * kAlphaCount));
    constexpr auto b =
        static_cast<Blending>((i / kAlphaCount) % kBlendingCount);
    constexpr auto a = static_cast<Alpha>(i % kAlphaCount);
    return MakeBlendFn<c, b, a>();
  }()...};
}

inline constexpr auto kDispatchTableSize =
    kColorCount * kBlendingCount * kAlphaCount;

// --- Dispatch: static table instance ---------------------------------------

inline constexpr auto kDispatchTable =
    BuildDispatchTable(std::make_index_sequence<kDispatchTableSize>{});

// ═══════════════════════════════════════════════════════
//                  Internal BlendInPlace
// ═══════════════════════════════════════════════════════

inline void BlendInPlaceImpl(Buffer &dst, const Buffer &src,
                             std::ptrdiff_t x_off, std::ptrdiff_t y_off,
                             Blending blending, Alpha alpha,
                             parallel::ExecutionPolicy exec) {
  assert(dst.Color() == src.Color());

  auto color = dst.Color();

  if (color == Color::kCmyk) {
    auto dst_rgb = image::ConvertColor(dst, Color::kRgb);
    auto src_rgb = image::ConvertColor(src, Color::kRgb);
    BlendInPlaceImpl(dst_rgb, src_rgb, x_off, y_off, blending, alpha, exec);
    image::ConvertColor(dst_rgb, dst);
    return;
  }

  auto region = Region::Compute(dst.Width(), dst.Height(), src.Width(),
                                src.Height(), x_off, y_off);
  if (region.Empty()) return;

  auto index = static_cast<int>(color) * kBlendingCount * kAlphaCount +
               static_cast<int>(blending) * kAlphaCount +
               static_cast<int>(alpha);

  auto fn = kDispatchTable[static_cast<std::size_t>(index)];
  assert(fn != nullptr);

  fn(dst.Data(), dst.Width() * dst.ChannelCount(), src.Data(),
     src.Width() * src.ChannelCount(), region, exec);
}

#if WQIMAGE_SIMD
}  // namespace HWY_NAMESPACE
#endif
}  // namespace weqeqq::image::internal

#if WQIMAGE_SIMD
HWY_AFTER_NAMESPACE();

#if HWY_ONCE

namespace weqeqq::image {
namespace internal {
HWY_EXPORT(PremultiplyInPlace);
HWY_EXPORT(UnpremultiplyInPlace);
HWY_EXPORT(BlendInPlaceImpl);
}  // namespace internal

#ifndef __CLANGD__
void PremultiplyInPlace(Buffer &buffer,
                        parallel::ExecutionPolicy execution) noexcept(!kDebug) {
  HWY_DYNAMIC_DISPATCH(internal::PremultiplyInPlace)(buffer, execution);
}

void UnpremultiplyInPlace(
    Buffer &buffer, parallel::ExecutionPolicy execution) noexcept(!kDebug) {
  HWY_DYNAMIC_DISPATCH(internal::UnpremultiplyInPlace)(buffer, execution);
}

void BlendInPlace(Buffer &destination, const Buffer &source,
                  std::ptrdiff_t x_offset, std::ptrdiff_t y_offset,
                  Blending blending, Alpha alpha,
                  parallel::ExecutionPolicy execution) noexcept(!kDebug) {
  HWY_DYNAMIC_DISPATCH(internal::BlendInPlaceImpl)(
      destination, source, x_offset, y_offset, blending, alpha, execution);
}
#endif

}  // namespace weqeqq::image

#endif  // HWY_ONCE
#else   // WQIMAGE_SIMD

namespace weqeqq::image {

void PremultiplyInPlace(Buffer &buffer,
                        parallel::ExecutionPolicy execution) noexcept(!kDebug) {
  internal::PremultiplyInPlace(buffer, execution);
}

void UnpremultiplyInPlace(
    Buffer &buffer, parallel::ExecutionPolicy execution) noexcept(!kDebug) {
  internal::UnpremultiplyInPlace(buffer, execution);
}

void BlendInPlace(Buffer &destination, const Buffer &source,
                  std::ptrdiff_t x_offset, std::ptrdiff_t y_offset,
                  Blending blending, Alpha alpha,
                  parallel::ExecutionPolicy execution) noexcept(!kDebug) {
  internal::BlendInPlaceImpl(destination, source, x_offset, y_offset, blending,
                             alpha, execution);
}

}  // namespace weqeqq::image

#endif  // WQIMAGE_SIMD
