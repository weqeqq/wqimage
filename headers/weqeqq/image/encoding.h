
#pragma once

#include <weqeqq/image/buffer.h>
#include <weqeqq/image/error.h>
#include <weqeqq/image/export.h>

#include <filesystem>
#include <vector>

namespace weqeqq::image {

struct WQIMAGE_EXPORT EncodingError : Error {
  using Error::Error;
};

struct WQIMAGE_EXPORT PngEncodingError : EncodingError {
  using EncodingError::EncodingError;
};

enum class Format {
  kPng,
};

WQIMAGE_EXPORT std::vector<std::uint8_t> Encode(const Buffer &buffer,
                                                Format format);

WQIMAGE_EXPORT void Encode(const Buffer &buffer, Format format,
                           const std::filesystem::path &filename);

WQIMAGE_EXPORT void Encode(const Buffer &buffer,
                           const std::filesystem::path &filename);

}  // namespace weqeqq::image
