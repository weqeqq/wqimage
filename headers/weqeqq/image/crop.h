#pragma once

#include <weqeqq/image/buffer.h>
#include <weqeqq/image/error.h>
#include <weqeqq/image/export.h>

#include <cstddef>
#include <format>
#include <source_location>

namespace weqeqq::image {

struct WQIMAGE_EXPORT CropError : Error {
  using Error::Error;
};

struct WQIMAGE_EXPORT CropInvalidSizeError : CropError {
  CropInvalidSizeError(
      std::size_t width, std::size_t height,
      std::source_location location = std::source_location::current())
      : CropError(std::format("Invalid crop dimensions: {}x{} (must be > 0)",
                              width, height),
                  location) {}
};

struct WQIMAGE_EXPORT CropRegionOutOfBoundsError : CropError {
  CropRegionOutOfBoundsError(
      std::size_t x, std::size_t y, std::size_t width, std::size_t height,
      std::size_t image_width, std::size_t image_height,
      std::source_location location = std::source_location::current())
      : CropError(
            std::format(
                "Crop region ({}, {}) {}x{} is out of bounds for image {}x{}",
                x, y, width, height, image_width, image_height),
            location) {}
};

[[nodiscard]]
WQIMAGE_EXPORT Buffer Crop(const Buffer& input, std::size_t x, std::size_t y,
                           std::size_t width, std::size_t height);

}  // namespace weqeqq::image
