module;

#include <string>
#include <system_error>
#include <type_traits>

export module weqeqq.image:error;

export import weqeqq.error;

export namespace weqeqq::image {

enum class ImageError {
  kBufferUninitialized = 1,
  kBufferLinearIndexOutOfBounds,
  kBufferCoordinateOutOfBounds,
  kBufferPixelCountOverflow,
  kBufferStrideOverflow,
  kBufferByteCountOverflow,
  kBufferInvalidDimensions,
  kBufferDataSizeMismatch,
  kBufferZeroDimensionsInvariant,
  kBufferInvalidColorInvariant,
  kBufferByteCountInvariant,
};

class ImageErrorCategory : public std::error_category {
 public:
  [[nodiscard]] const char* name() const noexcept override {
    return "weqeqq.image";
  }

  [[nodiscard]] std::string message(int value) const override {
    switch (static_cast<ImageError>(value)) {
      case ImageError::kBufferUninitialized:
        return "buffer is uninitialized";
      case ImageError::kBufferLinearIndexOutOfBounds:
        return "buffer linear index is out of bounds";
      case ImageError::kBufferCoordinateOutOfBounds:
        return "buffer coordinates are out of bounds";
      case ImageError::kBufferPixelCountOverflow:
        return "buffer pixel count calculation overflowed";
      case ImageError::kBufferStrideOverflow:
        return "buffer stride calculation overflowed";
      case ImageError::kBufferByteCountOverflow:
        return "buffer byte count calculation overflowed";
      case ImageError::kBufferInvalidDimensions:
        return "buffer dimensions are invalid";
      case ImageError::kBufferDataSizeMismatch:
        return "buffer data size does not match metadata";
      case ImageError::kBufferZeroDimensionsInvariant:
        return "buffer invariant violated: zero dimensions";
      case ImageError::kBufferInvalidColorInvariant:
        return "buffer invariant violated: invalid color";
      case ImageError::kBufferByteCountInvariant:
        return "buffer invariant violated: byte count mismatch";
    }
    return "unknown weqeqq.image error";
  }
};

[[nodiscard]] inline const std::error_category& image_category() noexcept {
  static const ImageErrorCategory category;
  return category;
}

[[nodiscard]] inline std::error_code make_error_code(ImageError error) noexcept {
  return {static_cast<int>(error), image_category()};
}

}  // namespace weqeqq::image

namespace std {
template <>
struct is_error_code_enum<weqeqq::image::ImageError> : true_type {};
}  // namespace std
