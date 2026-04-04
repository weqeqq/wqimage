
#include <weqeqq/file.h>
#include <weqeqq/image/encoding.h>
#include <weqeqq/png.h>

#include <cstdint>
#include <filesystem>
#include <vector>

namespace weqeqq::image {

namespace {
Format FormatFromFilename(const std::filesystem::path &filename) {
  auto extension = filename.extension().string();
  if (extension == ".png") {
    return Format::kPng;
  } else {
    throw EncodingError("Unsupported format for");
  }
}
std::vector<std::uint8_t> EncodePng(const Buffer &image) {
  return png::EncodeImage(image, image.Width(), image.Height(), image.Color());
}
}  // namespace

std::vector<std::uint8_t> Encode(const Buffer &buffer, Format format) {
  switch (format) {
    case Format::kPng:
      return EncodePng(buffer);
    default:
      throw EncodingError("Unsupported d format");
  }
}

void Encode(const Buffer &buffer, Format format,
            const std::filesystem::path &filename) {
  file::WriteBinary(Encode(buffer, format), filename);
}

void Encode(const Buffer &buffer, const std::filesystem::path &filename) {
  Encode(buffer, FormatFromFilename(filename), filename);
}

}  // namespace weqeqq::image
