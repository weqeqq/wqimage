
#pragma once

#include <weqeqq/image/buffer.h>
#include <weqeqq/image/error.h>
#include <weqeqq/image/export.h>

#include <filesystem>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

namespace weqeqq::image {

struct WQIMAGE_EXPORT EncodeError : Error {
  using Error::Error;
};

enum class Format {
  kPng,
};

struct WQIMAGE_EXPORT EncodeUnsupportedFormatError : EncodeError {
  EncodeUnsupportedFormatError(
      Format format, std::size_t buffer_width, std::size_t buffer_height,
      Color color,
      std::source_location location = std::source_location::current())
      : EncodeError(
            ErrorCode::kEncodeUnsupportedFormat,
            {"unsupported output image format",
             {
                 {"format", static_cast<int>(format)},
                 {"buffer_width", buffer_width},
                 {"buffer_height", buffer_height},
                 {"buffer_color", static_cast<int>(color)},
             },
             "The requested encoder format is not implemented by wqimage.",
             "Use Format::kPng or add an encoder implementation for the "
             "requested format.",
             location}) {}
};

struct WQIMAGE_EXPORT EncodeUnsupportedPathExtensionError : EncodeError {
  EncodeUnsupportedPathExtensionError(
      const std::filesystem::path &filename,
      std::source_location location = std::source_location::current())
      : EncodeError(
            ErrorCode::kEncodeUnsupportedPathExtension,
            {"unsupported output path extension",
             {
                 {"filename", filename.string()},
                 {"extension", filename.extension().empty()
                                   ? std::string("<none>")
                                   : filename.extension().string()},
             },
             "wqimage selects an encoder from the output path extension, and "
             "this extension is not supported.",
             "Use a .png filename or call Encode(buffer, Format::kPng, ...) "
             "explicitly.",
             location}) {}
};

struct WQIMAGE_EXPORT EncodeOutputWriteError : EncodeError {
  EncodeOutputWriteError(
      const std::filesystem::path &filename, Format format,
      std::string_view cause,
      std::source_location location = std::source_location::current())
      : EncodeError(
            ErrorCode::kEncodeOutputWrite,
            {"failed to write encoded image",
             {
                 {"filename", filename.string()},
                 {"format", static_cast<int>(format)},
                 {"cause", std::string(cause)},
             },
             "Image encoding succeeded, but the encoded bytes could not be "
             "written to the destination path.",
             "Ensure the parent directory exists and the process has "
             "permission to create or overwrite the output file.",
             location}) {}
};

struct WQIMAGE_EXPORT EncodePngError : EncodeError {
  EncodePngError(
      std::size_t width, std::size_t height, Color color,
      std::string_view cause,
      std::source_location location = std::source_location::current())
      : EncodeError(
            ErrorCode::kEncodePng,
            {"png encode failed",
             {
                 {"width", width},
                 {"height", height},
                 {"color", static_cast<int>(color)},
                 {"cause", std::string(cause)},
             },
             "The underlying PNG encoder rejected the image or failed while "
             "producing PNG output.",
             "Verify that the buffer metadata and color format are supported "
             "by the PNG encoder.",
             location}) {}
};

WQIMAGE_EXPORT std::vector<std::uint8_t> Encode(const Buffer &buffer,
                                                Format format);

WQIMAGE_EXPORT void Encode(const Buffer &buffer, Format format,
                           const std::filesystem::path &filename);

WQIMAGE_EXPORT void Encode(const Buffer &buffer,
                           const std::filesystem::path &filename);

}  // namespace weqeqq::image
