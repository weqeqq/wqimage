
#include <weqeqq/file.h>
#include <weqeqq/image/decoding.h>
#include <weqeqq/png.h>

#include <cstdint>
#include <vector>

namespace weqeqq::image {

namespace {

Buffer DecodePng(std::span<const std::uint8_t> binary, Color color) {
  auto [data, info] = png::DecodeImage(binary, color);

  return Buffer(info.width, info.height, color, std::move(data));
}

}  // namespace

Buffer Decode(const std::filesystem::path &filename, Color color) {
  return Decode(file::ReadBinary(filename), color);
}

Buffer Decode(std::span<const std::uint8_t> data, Color color) {
  if (png::HasPngSignature(data)) {
    return DecodePng(data, color);
  } else {
    throw DecodingError("Unsupported format");
  }
}

}  // namespace weqeqq::image
