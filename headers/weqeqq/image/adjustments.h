#pragma once

#include <weqeqq/image/buffer.h>
#include <weqeqq/image/config.h>
#include <weqeqq/image/error.h>
#include <weqeqq/image/export.h>
#include <weqeqq/parallel.h>

#include <format>
#include <source_location>

namespace weqeqq::image {

enum class AdjustmentMode {
  kModern,
  kLegacy,

  kCount,
};

inline constexpr int kBrightnessMin = -150;
inline constexpr int kBrightnessMax = 150;
inline constexpr int kContrastMin   = -50;
inline constexpr int kContrastMax   = 100;

struct WQIMAGE_EXPORT AdjustmentError : Error {
  using Error::Error;
};

struct WQIMAGE_EXPORT AdjustmentUnsupportedColorError : AdjustmentError {
  AdjustmentUnsupportedColorError(
      Color color,
      std::source_location location = std::source_location::current())
      : AdjustmentError(
            ErrorCode::kAdjustmentUnsupportedColor,
            {"unsupported color format for image adjustment",
             {
                 {"color", static_cast<int>(color)},
             },
             "Brightness and contrast adjustments only support Grayscale, RGB, "
             "and RGBA buffers.",
             "Convert the buffer to Grayscale, RGB, or RGBA before adjusting "
             "it.",
             location}) {}
};

struct WQIMAGE_EXPORT AdjustmentBrightnessOutOfRangeError : AdjustmentError {
  AdjustmentBrightnessOutOfRangeError(
      int value, int min, int max, AdjustmentMode mode,
      std::source_location location = std::source_location::current())
      : AdjustmentError(
            ErrorCode::kAdjustmentBrightnessOutOfRange,
            {"brightness value is out of range",
             {
                 {"value", value},
                 {"min", min},
                 {"max", max},
                 {"mode", static_cast<int>(mode)},
             },
             "Brightness accepts only values inside the supported slider "
             "range.",
             "Pass a brightness value between the documented minimum and "
             "maximum for this API.",
             location}) {}
};

struct WQIMAGE_EXPORT AdjustmentContrastOutOfRangeError : AdjustmentError {
  AdjustmentContrastOutOfRangeError(
      int value, int min, int max, AdjustmentMode mode,
      std::source_location location = std::source_location::current())
      : AdjustmentError(
            ErrorCode::kAdjustmentContrastOutOfRange,
            {"contrast value is out of range",
             {
                 {"value", value},
                 {"min", min},
                 {"max", max},
                 {"mode", static_cast<int>(mode)},
             },
             "Contrast accepts only values inside the supported slider range.",
             "Pass a contrast value between the documented minimum and "
             "maximum for this API.",
             location}) {}
};

struct WQIMAGE_EXPORT AdjustmentInvalidModeError : AdjustmentError {
  AdjustmentInvalidModeError(
      AdjustmentMode mode,
      std::source_location location = std::source_location::current())
      : AdjustmentError(
            ErrorCode::kAdjustmentInvalidMode,
            {"adjustment mode is invalid",
             {
                 {"mode", static_cast<int>(mode)},
             },
             "The supplied adjustment mode is not a supported runtime enum "
             "value.",
             "Use AdjustmentMode::kModern or AdjustmentMode::kLegacy.",
             location}) {}
};

[[nodiscard]]
WQIMAGE_EXPORT Buffer AdjustBrightness(
    const Buffer& input, int value,
    AdjustmentMode mode                 = AdjustmentMode::kModern,
    parallel::ExecutionPolicy execution = parallel::Execution::kSequential);

WQIMAGE_EXPORT void AdjustBrightnessInPlace(
    Buffer& buffer, int value, AdjustmentMode mode = AdjustmentMode::kModern,
    parallel::ExecutionPolicy execution = parallel::Execution::kSequential);

[[nodiscard]]
WQIMAGE_EXPORT Buffer AdjustContrast(
    const Buffer& input, int value,
    AdjustmentMode mode                 = AdjustmentMode::kModern,
    parallel::ExecutionPolicy execution = parallel::Execution::kSequential);

WQIMAGE_EXPORT void AdjustContrastInPlace(
    Buffer& buffer, int value, AdjustmentMode mode = AdjustmentMode::kModern,
    parallel::ExecutionPolicy execution = parallel::Execution::kSequential);

}  // namespace weqeqq::image
