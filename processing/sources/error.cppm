module;

#include <string>
#include <system_error>
#include <type_traits>

export module weqeqq.image.processing:error;

export import weqeqq.error;

export namespace weqeqq::image {

enum class ProcessingError {
  kCropInvalidDimensions = 1,
  kCropOriginOutOfBounds,
  kCropExtentOutOfBounds,
  kAdjustmentUnsupportedColor,
  kAdjustmentBrightnessOutOfRange,
  kAdjustmentContrastOutOfRange,
  kAdjustmentInvalidMode,
  kBlendOpacityOutOfRange,
  kResizeEmptyInput,
  kResizeInvalidDimensions,
  kResizeInvalidFilter,
  kResizeInvalidScale,
};

class ProcessingErrorCategory : public std::error_category {
 public:
  [[nodiscard]] const char* name() const noexcept override {
    return "weqeqq.image.processing";
  }

  [[nodiscard]] std::string message(int value) const override {
    switch (static_cast<ProcessingError>(value)) {
      case ProcessingError::kCropInvalidDimensions:
        return "crop dimensions are invalid";
      case ProcessingError::kCropOriginOutOfBounds:
        return "crop origin is out of bounds";
      case ProcessingError::kCropExtentOutOfBounds:
        return "crop extent is out of bounds";
      case ProcessingError::kAdjustmentUnsupportedColor:
        return "adjustments do not support this color format";
      case ProcessingError::kAdjustmentBrightnessOutOfRange:
        return "brightness value is out of range";
      case ProcessingError::kAdjustmentContrastOutOfRange:
        return "contrast value is out of range";
      case ProcessingError::kAdjustmentInvalidMode:
        return "adjustment mode is invalid";
      case ProcessingError::kBlendOpacityOutOfRange:
        return "blend opacity is out of range";
      case ProcessingError::kResizeEmptyInput:
        return "resize input buffer is empty";
      case ProcessingError::kResizeInvalidDimensions:
        return "resize target dimensions are invalid";
      case ProcessingError::kResizeInvalidFilter:
        return "resize filter is invalid";
      case ProcessingError::kResizeInvalidScale:
        return "resize scale factor is invalid";
    }
    return "unknown weqeqq.image.processing error";
  }
};

[[nodiscard]] inline const std::error_category& processing_category() noexcept {
  static const ProcessingErrorCategory category;
  return category;
}

[[nodiscard]] inline std::error_code make_error_code(
    ProcessingError error) noexcept {
  return {static_cast<int>(error), processing_category()};
}

}  // namespace weqeqq::image

namespace std {
template <>
struct is_error_code_enum<weqeqq::image::ProcessingError>
    : true_type {};
}  // namespace std
