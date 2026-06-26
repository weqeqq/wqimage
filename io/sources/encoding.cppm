module;

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

export module weqeqq.image.io:encoding;

import :error;
export import weqeqq.image;

export namespace weqeqq::image {

enum class Format {
  kPng,
  kAvif,
};

}  // namespace weqeqq::image

namespace weqeqq::image {

namespace internal {

// Defined in encoding.cpp, which owns the BMI dependency on weqeqq.png and
// weqeqq.avif so it does not leak to consumers of weqeqq.image.io.
std::vector<std::uint8_t> EncodePng(const Buffer& buffer);
std::vector<std::uint8_t> EncodeAvif(const Buffer& buffer);

[[noreturn]] void ThrowUnsupportedFormat(Format format, const Buffer& buffer) {
  throw error::TypedErrorBuilder(IoError::kEncodeUnsupportedFormat,
                                 "unsupported output image format")
      .Field("format", static_cast<int>(format))
      .Field("buffer_width", buffer.Width())
      .Field("buffer_height", buffer.Height())
      .Field("buffer_color", static_cast<int>(buffer.Color()))
      .Details("The requested encoder format is not implemented by wqimage.")
      .Build();
}

[[noreturn]] void ThrowUnsupportedPathExtension(
    const std::filesystem::path& filename) {
  throw error::TypedErrorBuilder(IoError::kEncodeUnsupportedPathExtension,
                                 "unsupported output path extension")
      .Field("filename", filename.string())
      .Field("extension", filename.extension().empty()
                              ? std::string("<none>")
                              : filename.extension().string())
      .Details(
          "wqimage selects an encoder from the output path extension, and this "
          "extension is not supported.")
      .Hint("Use a .png or .avif filename, or call Encode with an explicit "
            "Format.")
      .Build();
}

[[noreturn]] void ThrowEncodeFailed(const Buffer& buffer, const char* cause) {
  throw error::TypedErrorBuilder(IoError::kEncodeFailed, "image encode failed")
      .Field("width", buffer.Width())
      .Field("height", buffer.Height())
      .Field("color", static_cast<int>(buffer.Color()))
      .Details(cause)
      .Hint(
          "Verify that the buffer metadata and color format are supported by "
          "the selected encoder.")
      .Build();
}

[[noreturn]] void ThrowOutputWriteError(const std::filesystem::path& filename,
                                        Format format, const char* cause) {
  throw error::TypedErrorBuilder(IoError::kEncodeOutputWrite,
                                 "failed to write encoded image")
      .Field("filename", filename.string())
      .Field("format", static_cast<int>(format))
      .Details(cause)
      .Hint(
          "Ensure the parent directory exists and the process has permission "
          "to create or overwrite the output file.")
      .Build();
}

void WriteFile(const std::vector<std::uint8_t>& data, Format format,
               const std::filesystem::path& filename) {
  std::ofstream stream(filename, std::ios::binary);
  if (!stream) {
    ThrowOutputWriteError(filename, format,
                          "the file could not be opened for writing");
  }
  stream.write(reinterpret_cast<const char*>(data.data()),
               static_cast<std::streamsize>(data.size()));
  if (!stream) {
    ThrowOutputWriteError(filename, format, "the encoded bytes could not be "
                                            "written to the destination");
  }
}

Format FormatFromFilename(const std::filesystem::path& filename) {
  const auto extension = filename.extension().string();
  if (extension == ".png") {
    return Format::kPng;
  }
  if (extension == ".avif") {
    return Format::kAvif;
  }
  ThrowUnsupportedPathExtension(filename);
}

}  // namespace internal

export std::vector<std::uint8_t> Encode(const Buffer& buffer, Format format) {
  switch (format) {
    case Format::kPng:
      return internal::EncodePng(buffer);
    case Format::kAvif:
      return internal::EncodeAvif(buffer);
  }
  internal::ThrowUnsupportedFormat(format, buffer);
}

export void Encode(const Buffer& buffer, Format format,
                   const std::filesystem::path& filename) {
  internal::WriteFile(Encode(buffer, format), format, filename);
}

export void Encode(const Buffer& buffer,
                   const std::filesystem::path& filename) {
  Encode(buffer, internal::FormatFromFilename(filename), filename);
}

}  // namespace weqeqq::image
