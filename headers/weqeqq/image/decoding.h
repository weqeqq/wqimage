
#pragma once

#include <weqeqq/image/buffer.h>
#include <weqeqq/image/error.h>
#include <weqeqq/image/export.h>

#include <cstdint>
#include <filesystem>
#include <source_location>
#include <string>
#include <string_view>

namespace weqeqq::image {

struct WQIMAGE_EXPORT DecodeError : Error {
  using Error::Error;
};

struct WQIMAGE_EXPORT DecodeUnsupportedFormatError : DecodeError {
  DecodeUnsupportedFormatError(
      std::size_t input_bytes, Color color,
      std::source_location location = std::source_location::current())
      : DecodeError(
            ErrorCode::kDecodeUnsupportedFormat,
            {"unsupported input image format",
             {
                 {"input_bytes", input_bytes},
                 {"requested_color", static_cast<int>(color)},
             },
             "The input bytes do not match any image signature supported by "
             "wqimage decoding.",
             "Provide PNG data or route the input through a decoder that "
             "supports this format.",
             location}) {}

  DecodeUnsupportedFormatError(
      const std::filesystem::path& filename, std::size_t input_bytes,
      Color color,
      std::source_location location = std::source_location::current())
      : DecodeError(
            ErrorCode::kDecodeUnsupportedFormat,
            {"unsupported input image format",
             {
                 {"filename", filename.string()},
                 {"input_bytes", input_bytes},
                 {"requested_color", static_cast<int>(color)},
             },
             "The file was read successfully, but its contents do not match "
             "any image signature supported by wqimage decoding.",
             "Provide a PNG file or route the input through a decoder that "
             "supports this format.",
             location}) {}
};

struct WQIMAGE_EXPORT DecodeInputReadError : DecodeError {
  DecodeInputReadError(
      const std::filesystem::path& filename, std::string_view cause,
      std::source_location location = std::source_location::current())
      : DecodeError(
            ErrorCode::kDecodeInputRead,
            {"failed to read input image",
             {
                 {"filename", filename.string()},
                 {"cause", std::string(cause)},
             },
             "The input file could not be opened or read before decoding "
             "started.",
             "Check that the path exists, points to a regular file, and is "
             "readable by the current process.",
             location}) {}
};

struct WQIMAGE_EXPORT DecodePngError : DecodeError {
  DecodePngError(
      std::size_t input_bytes, Color color, std::string_view cause,
      std::source_location location = std::source_location::current())
      : DecodeError(
            ErrorCode::kDecodePng,
            {"png decode failed",
             {
                 {"input_bytes", input_bytes},
                 {"requested_color", static_cast<int>(color)},
                 {"cause", std::string(cause)},
             },
             "The input was identified as PNG, but the PNG decoder failed to "
             "produce image data.",
             "Check that the PNG data is complete, valid, and supported by "
             "the bundled decoder.",
             location}) {}

  DecodePngError(
      const std::filesystem::path& filename, std::size_t input_bytes,
      Color color, std::string_view cause,
      std::source_location location = std::source_location::current())
      : DecodeError(
            ErrorCode::kDecodePng,
            {"png decode failed",
             {
                 {"filename", filename.string()},
                 {"input_bytes", input_bytes},
                 {"requested_color", static_cast<int>(color)},
                 {"cause", std::string(cause)},
             },
             "The input was identified as PNG, but the PNG decoder failed to "
             "produce image data.",
             "Check that the PNG file is complete, valid, and supported by "
             "the bundled decoder.",
             location}) {}
};

WQIMAGE_EXPORT Buffer Decode(const std::filesystem::path& filename,
                             Color color = Color::kRgb);

WQIMAGE_EXPORT Buffer Decode(std::span<const std::uint8_t> data,
                             Color color = Color::kRgb);

};  // namespace weqeqq::image
