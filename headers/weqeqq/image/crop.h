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

struct WQIMAGE_EXPORT CropInvalidDimensionsError : CropError {
  CropInvalidDimensionsError(
      std::size_t width, std::size_t height,
      std::source_location location = std::source_location::current())
      : CropError(ErrorCode::kCropInvalidDimensions,
                  {"invalid crop dimensions",
                   {
                       {"width", width},
                       {"height", height},
                   },
                   "Crop dimensions must be positive.",
                   "Pass non-zero width and height for the crop region.",
                   location}) {}
};

struct WQIMAGE_EXPORT CropOriginOutOfBoundsError : CropError {
  CropOriginOutOfBoundsError(
      std::size_t x, std::size_t y, std::size_t width, std::size_t height,
      std::size_t image_width, std::size_t image_height,
      std::source_location location = std::source_location::current())
      : CropError(
            ErrorCode::kCropOriginOutOfBounds,
            {"crop origin is out of bounds",
             {
                 {"x", x},
                 {"y", y},
                 {"width", width},
                 {"height", height},
                 {"image_width", image_width},
                 {"image_height", image_height},
             },
             "The requested crop origin does not lie inside the source image.",
             "Choose x and y so they fall within the image bounds before "
             "requesting the crop extent.",
             location}) {}
};

struct WQIMAGE_EXPORT CropExtentOutOfBoundsError : CropError {
  CropExtentOutOfBoundsError(
      std::size_t x, std::size_t y, std::size_t width, std::size_t height,
      std::size_t image_width, std::size_t image_height,
      std::source_location location = std::source_location::current())
      : CropError(ErrorCode::kCropExtentOutOfBounds,
                  {"crop extent is out of bounds",
                   {
                       {"x", x},
                       {"y", y},
                       {"width", width},
                       {"height", height},
                       {"image_width", image_width},
                       {"image_height", image_height},
                   },
                   "The requested crop extent does not fit within the source "
                   "image starting from the supplied origin.",
                   "Reduce the crop width or height, or choose a different "
                   "origin inside the image bounds.",
                   location}) {}
};

[[nodiscard]]
WQIMAGE_EXPORT Buffer Crop(const Buffer& input, std::size_t x, std::size_t y,
                           std::size_t width, std::size_t height);

}  // namespace weqeqq::image
