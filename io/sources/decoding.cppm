module;

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <span>
#include <system_error>
#include <utility>
#include <vector>

export module weqeqq.image.io:decoding;

import :error;
export import weqeqq.image;

import weqeqq.png;
import weqeqq.avif;

namespace weqeqq::image {

namespace internal {

[[noreturn]] void ThrowUnsupportedFormat(std::size_t input_bytes, Color color) {
  throw error::TypedErrorBuilder(IoError::kDecodeUnsupportedFormat,
                                 "unsupported input image format")
      .Field("input_bytes", input_bytes)
      .Field("requested_color", static_cast<int>(color))
      .Details(
          "The input bytes do not match any image signature supported by "
          "wqimage decoding.")
      .Hint("Provide PNG or AVIF data.")
      .Build();
}

[[noreturn]] void ThrowDecodeFailed(std::size_t input_bytes, Color color,
                                    const char* cause) {
  throw error::TypedErrorBuilder(IoError::kDecodeFailed, "image decode failed")
      .Field("input_bytes", input_bytes)
      .Field("requested_color", static_cast<int>(color))
      .Details(cause)
      .Hint("Check that the encoded data is complete, valid, and supported.")
      .Build();
}

[[noreturn]] void ThrowInputReadError(const std::filesystem::path& filename,
                                      const char* cause) {
  throw error::TypedErrorBuilder(IoError::kDecodeInputRead,
                                 "failed to read input image")
      .Field("filename", filename.string())
      .Details(cause)
      .Hint(
          "Check that the path exists, points to a regular file, and is "
          "readable by the current process.")
      .Build();
}

std::vector<std::uint8_t> ReadFile(const std::filesystem::path& filename) {
  std::error_code ec;
  if (!std::filesystem::exists(filename, ec) ||
      !std::filesystem::is_regular_file(filename, ec)) {
    ThrowInputReadError(filename,
                        "the path does not exist or is not a regular file");
  }

  std::ifstream stream(filename, std::ios::binary | std::ios::ate);
  if (!stream) {
    ThrowInputReadError(filename, "the file could not be opened for reading");
  }

  const auto length = stream.tellg();
  stream.seekg(0, std::ios::beg);

  std::vector<std::uint8_t> data(static_cast<std::size_t>(length));
  if (!stream.read(reinterpret_cast<char*>(data.data()),
                   static_cast<std::streamsize>(data.size()))) {
    ThrowInputReadError(filename, "the file contents could not be read");
  }
  return data;
}

Buffer DecodePng(std::span<const std::uint8_t> data, Color color) {
  try {
    auto decoded = png::DecodeImage(data, color);
    return Buffer(decoded.info.width, decoded.info.height, color,
                  std::move(decoded.data));
  } catch (const error::Error& cause) {
    ThrowDecodeFailed(data.size(), color, cause.what());
  }
}

Buffer DecodeAvif(std::span<const std::uint8_t> data, Color color) {
  try {
    auto decoded = avif::DecodeImage(data, color);
    return Buffer(decoded.info.width, decoded.info.height, color,
                  std::move(decoded.data));
  } catch (const error::Error& cause) {
    ThrowDecodeFailed(data.size(), color, cause.what());
  }
}

}  // namespace internal

export Buffer Decode(std::span<const std::uint8_t> data,
                     Color color = Color::kRgb) {
  if (png::HasPngSignature(data)) {
    return internal::DecodePng(data, color);
  }
  if (avif::HasAvifSignature(data)) {
    return internal::DecodeAvif(data, color);
  }
  internal::ThrowUnsupportedFormat(data.size(), color);
}

export Buffer Decode(const std::filesystem::path& filename,
                     Color color = Color::kRgb) {
  const auto data = internal::ReadFile(filename);
  return Decode(std::span<const std::uint8_t>(data), color);
}

}  // namespace weqeqq::image
