module;

#include <algorithm>
#include <cstddef>

export module weqeqq.image.processing:crop;

import :error;
export import weqeqq.image;

namespace weqeqq::image {

namespace internal {

[[noreturn]] void ThrowCropInvalidDimensions(std::size_t width,
                                             std::size_t height) {
  throw error::TypedErrorBuilder(ProcessingError::kCropInvalidDimensions,
                                 "invalid crop dimensions")
      .Field("width", width)
      .Field("height", height)
      .Details("Crop dimensions must be positive.")
      .Hint("Pass non-zero width and height for the crop region.")
      .Build();
}

[[noreturn]] void ThrowCropOriginOutOfBounds(std::size_t x, std::size_t y,
                                             std::size_t width,
                                             std::size_t height,
                                             std::size_t image_width,
                                             std::size_t image_height) {
  throw error::TypedErrorBuilder(ProcessingError::kCropOriginOutOfBounds,
                                 "crop origin is out of bounds")
      .Field("x", x)
      .Field("y", y)
      .Field("width", width)
      .Field("height", height)
      .Field("image_width", image_width)
      .Field("image_height", image_height)
      .Details(
          "The requested crop origin does not lie inside the source image.")
      .Hint(
          "Choose x and y so they fall within the image bounds before "
          "requesting the crop extent.")
      .Build();
}

[[noreturn]] void ThrowCropExtentOutOfBounds(std::size_t x, std::size_t y,
                                             std::size_t width,
                                             std::size_t height,
                                             std::size_t image_width,
                                             std::size_t image_height) {
  throw error::TypedErrorBuilder(ProcessingError::kCropExtentOutOfBounds,
                                 "crop extent is out of bounds")
      .Field("x", x)
      .Field("y", y)
      .Field("width", width)
      .Field("height", height)
      .Field("image_width", image_width)
      .Field("image_height", image_height)
      .Details(
          "The requested crop extent does not fit within the source image "
          "starting from the supplied origin.")
      .Hint(
          "Reduce the crop width or height, or choose a different origin "
          "inside the image bounds.")
      .Build();
}

void ValidateCropRegion(const Buffer& input, std::size_t x, std::size_t y,
                        std::size_t width, std::size_t height) {
  if (width == 0 || height == 0) {
    ThrowCropInvalidDimensions(width, height);
  }

  const auto input_width  = input.Width();
  const auto input_height = input.Height();

  if (x >= input_width || y >= input_height) {
    ThrowCropOriginOutOfBounds(x, y, width, height, input_width, input_height);
  }
  if (width > input_width - x || height > input_height - y) {
    ThrowCropExtentOutOfBounds(x, y, width, height, input_width, input_height);
  }
}

}  // namespace internal

export [[nodiscard]] Buffer Crop(const Buffer& input, std::size_t x,
                                 std::size_t y, std::size_t width,
                                 std::size_t height) {
  internal::ValidateCropRegion(input, x, y, width, height);

  Buffer output(width, height, input.Color());
  const auto row_bytes = width * input.ChannelCount();

  for (std::size_t row = 0; row < height; ++row) {
    const auto src_offset =
        ((y + row) * input.Width() + x) * input.ChannelCount();
    const auto dst_offset = row * row_bytes;

    std::copy_n(input.Data() + src_offset, row_bytes, output.Data() + dst_offset);
  }

  return output;
}

}  // namespace weqeqq::image
