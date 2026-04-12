
#include <weqeqq/file.h>
#include <weqeqq/image/encoding.h>
#include <weqeqq/png.h>

#include <cstdint>
#include <filesystem>
#include <vector>

namespace weqeqq::image {

namespace {
Format FormatFromFilename(const std::filesystem::path &filename) {
  const auto extension = filename.extension().string();
  if (extension == ".png") {
    return Format::kPng;
  } else {
    throw EncodeUnsupportedPathExtensionError(filename);
  }
}

std::vector<std::uint8_t> EncodePng(const Buffer &image) {
  try {
    return png::EncodeImage(image, image.Width(), image.Height(),
                            image.Color());
  } catch (const png::Error &error) {
    throw EncodePngError(image.Width(), image.Height(), image.Color(),
                         error.what());
  }
}
}  // namespace

std::vector<std::uint8_t> Encode(const Buffer &buffer, Format format) {
  switch (format) {
    case Format::kPng:
      return EncodePng(buffer);
    default:
      throw EncodeUnsupportedFormatError(format, buffer.Width(),
                                         buffer.Height(), buffer.Color());
  }
}

void Encode(const Buffer &buffer, Format format,
            const std::filesystem::path &filename) {
  try {
    file::WriteBinary(Encode(buffer, format), filename);
  } catch (const file::Error &error) {
    throw EncodeOutputWriteError(filename, format, error.what());
  }
}

void Encode(const Buffer &buffer, const std::filesystem::path &filename) {
  Encode(buffer, FormatFromFilename(filename), filename);
}

}  // namespace weqeqq::image
