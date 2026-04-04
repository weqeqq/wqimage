
#pragma once

#include <weqeqq/image/buffer.h>
#include <weqeqq/image/config.h>
#include <weqeqq/image/export.h>
#include <weqeqq/parallel.h>

namespace weqeqq::image {

enum class Blending {
  /* Normal */
  kNormal,
  kDissolve,  // Special Case; Todo

  /* Darken Group */
  kDarken,
  kMultiply,
  kColorBurn,
  kLinearBurn,
  kDarkerColor,

  /* Lighten Group */
  kLighten,
  kScreen,
  kColorDodge,
  kLinearDodge,
  kLighterColor,

  /* Contrast Group */
  kOverlay,
  kSoftLight,
  kHardLight,
  kVividLight,
  kLinearLight,
  kPinLight,
  kHardMix,

  /* Comparative Group */
  kDifference,
  kExclusion,
  kSubtract,
  kDivide,

  /* HSL Group */
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

/***************************************************
 *                   Premultiply                   *
 ***************************************************/

WQIMAGE_EXPORT void PremultiplyInPlace(
    Buffer &buffer,
    parallel::ExecutionPolicy = parallel::Execution::kSequential) noexcept(
    !kDebug);

inline Buffer Premultiply(
    Buffer buffer, parallel::ExecutionPolicy execution =
                       parallel::Execution::kSequential) noexcept(!kDebug) {
  PremultiplyInPlace(buffer, execution);
  return buffer;
}

/***************************************************
 *                 Unpremultiply                   *
 ***************************************************/

WQIMAGE_EXPORT void UnpremultiplyInPlace(
    Buffer &buffer, parallel::ExecutionPolicy =
                        parallel::Execution::kSequential) noexcept(!kDebug);

inline Buffer Unpremultiply(
    Buffer buffer, parallel::ExecutionPolicy execution =
                       parallel::Execution::kSequential) noexcept(!kDebug) {
  UnpremultiplyInPlace(buffer, execution);
  return buffer;
}

/***************************************************
 *                      Blend                      *
 ***************************************************/

WQIMAGE_EXPORT void BlendInPlace(           //
    Buffer &destination,                    //
    const Buffer &source,                   //
    std::ptrdiff_t x_offset,                //
    std::ptrdiff_t y_offset,                //
    Blending blending = Blending::kNormal,  //
    Alpha alpha       = Alpha::kStraight,   //
    parallel::ExecutionPolicy execution =
        parallel::Execution::kSequential) noexcept(!kDebug);

inline void BlendInPlace(                   //
    Buffer &destination,                    //
    const Buffer &source,                   //
    Blending blending = Blending::kNormal,  //
    Alpha alpha       = Alpha::kStraight,   //
    parallel::ExecutionPolicy execution =
        parallel::Execution::kSequential) noexcept(!kDebug) {
  //
  return BlendInPlace(destination, source, 0, 0, blending, alpha, execution);
}

inline Buffer Blend(                        //
    Buffer destination,                     //
    const Buffer &source,                   //
    std::ptrdiff_t x_offset,                //
    std::ptrdiff_t y_offset,                //
    Blending blending = Blending::kNormal,  //
    Alpha alpha       = Alpha::kStraight,   //
    parallel::ExecutionPolicy execution =
        parallel::Execution::kSequential) noexcept(!kDebug) {
  //
  BlendInPlace(destination, source, x_offset, y_offset, blending, alpha,
               execution);
  return destination;
}

inline Buffer Blend(                        //
    Buffer destination,                     //
    const Buffer &source,                   //
    Blending blending = Blending::kNormal,  //
    Alpha alpha       = Alpha::kStraight,   //
    parallel::ExecutionPolicy execution =
        parallel::Execution::kSequential) noexcept(!kDebug) {
  //
  return Blend(std::move(destination), source, 0, 0, blending, alpha,
               execution);
}

}  // namespace weqeqq::image
