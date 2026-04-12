#include <weqeqq/image/crop.h>

#include <algorithm>

namespace weqeqq::image {

namespace {

void ValidateCropRegion(const Buffer& input, std::size_t x, std::size_t y,
                        std::size_t width, std::size_t height) {
  if (width == 0 || height == 0) {
    throw CropInvalidDimensionsError(width, height);
  }

  const auto input_width  = input.Width();
  const auto input_height = input.Height();

  if (x >= input_width || y >= input_height) {
    throw CropOriginOutOfBoundsError(x, y, width, height, input_width,
                                     input_height);
  }
  if (width > input_width - x || height > input_height - y) {
    throw CropExtentOutOfBoundsError(x, y, width, height, input_width,
                                     input_height);
  }
}

}  // namespace

Buffer Crop(const Buffer& input, std::size_t x, std::size_t y,
            std::size_t width, std::size_t height) {
  ValidateCropRegion(input, x, y, width, height);

  Buffer output(width, height, input.Color());
  const auto row_bytes = width * input.ChannelCount();

  for (std::size_t row = 0; row < height; ++row) {
    const auto src_offset =
        ((y + row) * input.Width() + x) * input.ChannelCount();
    const auto dst_offset = row * row_bytes;

    std::copy_n(input.Data() + src_offset, row_bytes,
                output.Data() + dst_offset);
  }

  return output;
}

}  // namespace weqeqq::image
