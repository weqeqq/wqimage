module;

#include <string>
#include <system_error>
#include <type_traits>

export module weqeqq.image.io:error;

export import weqeqq.error;

export namespace weqeqq::image {

enum class IoError {
  kDecodeUnsupportedFormat = 1,
  kDecodeInputRead,
  kDecodeFailed,
  kEncodeUnsupportedFormat,
  kEncodeUnsupportedPathExtension,
  kEncodeOutputWrite,
  kEncodeFailed,
};

class IoErrorCategory : public std::error_category {
 public:
  [[nodiscard]] const char* name() const noexcept override {
    return "weqeqq.image.io";
  }

  [[nodiscard]] std::string message(int value) const override {
    switch (static_cast<IoError>(value)) {
      case IoError::kDecodeUnsupportedFormat:
        return "input image format is unsupported";
      case IoError::kDecodeInputRead:
        return "failed to read input image";
      case IoError::kDecodeFailed:
        return "image decode failed";
      case IoError::kEncodeUnsupportedFormat:
        return "output image format is unsupported";
      case IoError::kEncodeUnsupportedPathExtension:
        return "output path extension is unsupported";
      case IoError::kEncodeOutputWrite:
        return "failed to write encoded image";
      case IoError::kEncodeFailed:
        return "image encode failed";
    }
    return "unknown weqeqq.image.io error";
  }
};

[[nodiscard]] inline const std::error_category& io_category() noexcept {
  static const IoErrorCategory category;
  return category;
}

[[nodiscard]] inline std::error_code make_error_code(IoError error) noexcept {
  return {static_cast<int>(error), io_category()};
}

}  // namespace weqeqq::image

namespace std {
template <>
struct is_error_code_enum<weqeqq::image::IoError> : true_type {};
}  // namespace std
