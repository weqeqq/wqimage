
#pragma once

#include <weqeqq/image/buffer.h>
#include <weqeqq/image/error.h>
#include <weqeqq/image/export.h>

#include <cstdint>
#include <filesystem>

namespace weqeqq::image {

struct WQIMAGE_EXPORT DecodingError : Error {
  using Error::Error;
};

WQIMAGE_EXPORT Buffer Decode(const std::filesystem::path &filename,
                             Color color = Color::kRgb);

WQIMAGE_EXPORT Buffer Decode(std::span<const std::uint8_t> data,
                             Color color = Color::kRgb);

};  // namespace weqeqq::image
