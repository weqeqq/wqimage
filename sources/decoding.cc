
#include <weqeqq/file.h>
#include <weqeqq/image/decoding.h>
#include <weqeqq/png.h>

#include <cstdint>
#include <vector>

namespace weqeqq::image {

namespace {

Buffer DecodePng(std::span<const std::uint8_t> binary, Color color,
                 const std::filesystem::path* filename = nullptr) {
  try {
    auto [data, info] = png::DecodeImage(binary, color);
    return Buffer(info.width, info.height, color, std::move(data));
  } catch (const png::Error& error) {
    if (filename != nullptr) {
      throw DecodePngError(*filename, binary.size(), color, error.what());
    }
    throw DecodePngError(binary.size(), color, error.what());
  }
}

}  // namespace

Buffer Decode(const std::filesystem::path& filename, Color color) {
  try {
    const auto data = file::ReadBinary(filename);
    if (!png::HasPngSignature(data)) {
      throw DecodeUnsupportedFormatError(filename, data.size(), color);
    }
    return DecodePng(data, color, &filename);
  } catch (const file::Error& error) {
    throw DecodeInputReadError(filename, error.what());
  }
}

Buffer Decode(std::span<const std::uint8_t> data, Color color) {
  if (png::HasPngSignature(data)) {
    return DecodePng(data, color);
  } else {
    throw DecodeUnsupportedFormatError(data.size(), color);
  }
}

}  // namespace weqeqq::image
