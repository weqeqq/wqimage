
#pragma once

#include <weqeqq/image/buffer.h>
#include <weqeqq/image/config.h>
#include <weqeqq/image/error.h>
#include <weqeqq/image/export.h>
#include <weqeqq/parallel.h>

#include <source_location>

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

inline constexpr int kBlendOpacityMin = 0;
inline constexpr int kBlendOpacityMax = 100;

struct WQIMAGE_EXPORT BlendError : Error {
  using Error::Error;
};

struct WQIMAGE_EXPORT BlendOpacityOutOfRangeError : BlendError {
  BlendOpacityOutOfRangeError(
      int value, int min, int max,
      std::source_location location = std::source_location::current())
      : BlendError(
            ErrorCode::kBlendOpacityOutOfRange,
            {"blend opacity is out of range",
             {
                 {"value", value},
                 {"min", min},
                 {"max", max},
             },
             "Blend opacity accepts only values inside the supported percent "
             "range.",
             "Pass an opacity between 0 and 100 inclusive.",
             location}) {}
};

/***************************************************
 *                   Premultiply                   *
 ***************************************************/

WQIMAGE_EXPORT void PremultiplyInPlace(
    Buffer &buffer, parallel::ExecutionPolicy =
                        parallel::Execution::kSequential) noexcept(!kDebug);

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

WQIMAGE_EXPORT void BlendInPlace(                             //
    Buffer &destination,                                      //
    const Buffer &source,                                     //
    std::ptrdiff_t x_offset,                                  //
    std::ptrdiff_t y_offset,                                  //
    Blending blending                   = Blending::kNormal,  //
    Alpha alpha                         = Alpha::kStraight,   //
    int opacity                         = kBlendOpacityMax,   //
    parallel::ExecutionPolicy execution = parallel::Execution::kSequential);

inline void BlendInPlace(                                     //
    Buffer &destination,                                      //
    const Buffer &source,                                     //
    Blending blending                   = Blending::kNormal,  //
    Alpha alpha                         = Alpha::kStraight,   //
    int opacity                         = kBlendOpacityMax,   //
    parallel::ExecutionPolicy execution = parallel::Execution::kSequential) {
  //
  return BlendInPlace(destination, source, 0, 0, blending, alpha, opacity,
                      execution);
}

inline Buffer Blend(                                          //
    Buffer destination,                                       //
    const Buffer &source,                                     //
    std::ptrdiff_t x_offset,                                  //
    std::ptrdiff_t y_offset,                                  //
    Blending blending                   = Blending::kNormal,  //
    Alpha alpha                         = Alpha::kStraight,   //
    int opacity                         = kBlendOpacityMax,   //
    parallel::ExecutionPolicy execution = parallel::Execution::kSequential) {
  //
  BlendInPlace(destination, source, x_offset, y_offset, blending, alpha,
               opacity, execution);
  return destination;
}

inline Buffer Blend(                                          //
    Buffer destination,                                       //
    const Buffer &source,                                     //
    Blending blending                   = Blending::kNormal,  //
    Alpha alpha                         = Alpha::kStraight,   //
    int opacity                         = kBlendOpacityMax,   //
    parallel::ExecutionPolicy execution = parallel::Execution::kSequential) {
  //
  return Blend(std::move(destination), source, 0, 0, blending, alpha, opacity,
               execution);
}

}  // namespace weqeqq::image
