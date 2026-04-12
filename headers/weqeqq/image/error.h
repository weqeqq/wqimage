
#pragma once

#include <weqeqq/error.h>

#include <string>
#include <system_error>
#include <type_traits>

namespace weqeqq::image {

enum class ErrorCode {
  kBufferUninitialized = 1,
  kBufferLinearIndexOutOfBounds,
  kBufferCoordinateOutOfBounds,
  kBufferPixelCountOverflow,
  kBufferStrideOverflow,
  kBufferByteCountOverflow,
  kBufferInvalidDimensions,
  kBufferDataSizeMismatch,
  kBufferZeroDimensionsInvariantViolation,
  kBufferInvalidColorInvariantViolation,
  kBufferByteCountInvariantViolation,
  kCropInvalidDimensions,
  kCropOriginOutOfBounds,
  kCropExtentOutOfBounds,
  kDecodeUnsupportedFormat,
  kDecodeInputRead,
  kDecodePng,
  kEncodeUnsupportedFormat,
  kEncodeUnsupportedPathExtension,
  kEncodeOutputWrite,
  kEncodePng,
};

}  // namespace weqeqq::image

namespace std {

template <>
struct is_error_code_enum<weqeqq::image::ErrorCode> : true_type {};

}  // namespace std

namespace weqeqq::image {

class ImageErrorCategory final : public std::error_category {
 public:
  const char* name() const noexcept override { return "weqeqq.image"; }

  std::string message(int value) const override {
    switch (static_cast<ErrorCode>(value)) {
      case ErrorCode::kBufferUninitialized:
        return "buffer is uninitialized";
      case ErrorCode::kBufferLinearIndexOutOfBounds:
        return "buffer linear index is out of bounds";
      case ErrorCode::kBufferCoordinateOutOfBounds:
        return "buffer coordinates are out of bounds";
      case ErrorCode::kBufferPixelCountOverflow:
        return "buffer pixel count calculation overflowed";
      case ErrorCode::kBufferStrideOverflow:
        return "buffer stride calculation overflowed";
      case ErrorCode::kBufferByteCountOverflow:
        return "buffer byte count calculation overflowed";
      case ErrorCode::kBufferInvalidDimensions:
        return "buffer dimensions are invalid";
      case ErrorCode::kBufferDataSizeMismatch:
        return "buffer data size does not match metadata";
      case ErrorCode::kBufferZeroDimensionsInvariantViolation:
        return "buffer invariant violated: zero dimensions";
      case ErrorCode::kBufferInvalidColorInvariantViolation:
        return "buffer invariant violated: invalid color";
      case ErrorCode::kBufferByteCountInvariantViolation:
        return "buffer invariant violated: byte count mismatch";
      case ErrorCode::kCropInvalidDimensions:
        return "crop dimensions are invalid";
      case ErrorCode::kCropOriginOutOfBounds:
        return "crop origin is out of bounds";
      case ErrorCode::kCropExtentOutOfBounds:
        return "crop extent is out of bounds";
      case ErrorCode::kDecodeUnsupportedFormat:
        return "input image format is unsupported";
      case ErrorCode::kDecodeInputRead:
        return "failed to read input image";
      case ErrorCode::kDecodePng:
        return "png decode failed";
      case ErrorCode::kEncodeUnsupportedFormat:
        return "output image format is unsupported";
      case ErrorCode::kEncodeUnsupportedPathExtension:
        return "output path extension is unsupported";
      case ErrorCode::kEncodeOutputWrite:
        return "failed to write encoded image";
      case ErrorCode::kEncodePng:
        return "png encode failed";
    }
    return "unknown image error";
  }
};

inline const std::error_category& GetErrorCategory() noexcept {
  static const ImageErrorCategory category;
  return category;
}

inline std::error_code make_error_code(ErrorCode code) noexcept {
  return {static_cast<int>(code), GetErrorCategory()};
}

struct Error : error::TypedError<ErrorCode> {
  using error::TypedError<ErrorCode>::TypedError;
};

}  // namespace weqeqq::image
