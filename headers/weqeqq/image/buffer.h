#pragma once

#include <weqeqq/color.h>
#include <weqeqq/image/error.h>
#include <weqeqq/image/export.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <source_location>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace weqeqq::image {

using Color = weqeqq::color::Format;

inline constexpr auto ColorCount = 4;

// ── Errors ───────────────────────────────────────────────────────

struct WQIMAGE_EXPORT BufferError : Error {
  using Error::Error;
};

struct WQIMAGE_EXPORT BufferNotInitializedError : BufferError {
  BufferNotInitializedError(
      std::source_location location = std::source_location::current())
      : BufferError("Buffer not initialized", location) {}
};

struct WQIMAGE_EXPORT BufferIndexOutOfRangeError : BufferError {
  BufferIndexOutOfRangeError(
      std::size_t index, std::size_t length,
      std::source_location location = std::source_location::current())
      : BufferError(std::format("Index {} out of range [0, {})", index, length),
                    location) {}

  BufferIndexOutOfRangeError(
      std::size_t row, std::size_t column, std::size_t height,
      std::size_t width,
      std::source_location location = std::source_location::current())
      : BufferError(
            std::format("Position ({}, {}) out of range [0, {}) x [0, {})", row,
                        column, height, width),
            location) {}
};

struct WQIMAGE_EXPORT BufferOverflowError : BufferError {
  BufferOverflowError(
      std::size_t a, std::size_t b,
      std::source_location location = std::source_location::current())
      : BufferError(
            std::format("Multiplication overflow: {} * {} exceeds size_t", a,
                        b),
            location) {}
};

struct WQIMAGE_EXPORT BufferInvalidSizeError : BufferError {
  BufferInvalidSizeError(
      std::size_t width, std::size_t height,
      std::source_location location = std::source_location::current())
      : BufferError(
            std::format("Invalid buffer dimensions: {}x{} (must be > 0)", width,
                        height),
            location) {}
};

struct WQIMAGE_EXPORT BufferDataSizeMismatchError : BufferError {
  BufferDataSizeMismatchError(
      std::size_t expected, std::size_t actual,
      std::source_location location = std::source_location::current())
      : BufferError(std::format("Data size mismatch: expected {} bytes, got {}",
                                expected, actual),
                    location) {}
};

struct WQIMAGE_EXPORT BufferInvariantError : BufferError {
  BufferInvariantError(
      std::string_view message,
      std::source_location location = std::source_location::current())
      : BufferError(message, location) {}
};

// ── BufferInfo ───────────────────────────────────────────────────

struct BufferInfo {
  std::size_t width  = 0;
  std::size_t height = 0;
  Color color        = static_cast<Color>(0);

  [[nodiscard]] std::size_t ChannelCount() const noexcept {
    return color::ChannelCount(color);
  }

  [[nodiscard]] std::size_t PixelCount() const {
    if (width == 0 || height == 0) {
      return 0;
    }
    if (width > std::numeric_limits<std::size_t>::max() / height) {
      throw BufferOverflowError(width, height);
    }
    return width * height;
  }

  [[nodiscard]] std::size_t StrideBytes() const {
    const auto channel_count = ChannelCount();
    if (width == 0 || channel_count == 0) {
      return 0;
    }
    if (width > std::numeric_limits<std::size_t>::max() / channel_count) {
      throw BufferOverflowError(width, channel_count);
    }
    return width * channel_count;
  }

  [[nodiscard]] std::size_t ByteCount() const {
    const auto stride = StrideBytes();
    if (height == 0 || stride == 0) {
      return 0;
    }
    if (height > std::numeric_limits<std::size_t>::max() / stride) {
      throw BufferOverflowError(height, stride);
    }
    return height * stride;
  }

  auto operator<=>(const BufferInfo& other) const noexcept = default;
};

// ── Buffer ───────────────────────────────────────────────────────

class Buffer {
  Buffer(BufferInfo info, std::vector<std::uint8_t> data)
      : info_{info}, data_{std::move(data)} {}

 public:
  static inline constexpr bool Debug =
#ifdef NDEBUG
      false;
#else
      true;
#endif

  Buffer() noexcept                = default;
  Buffer(const Buffer&)            = delete;
  Buffer& operator=(const Buffer&) = delete;

  Buffer(Buffer&& rhs) noexcept
      : info_{std::move(rhs.info_)}, data_{std::move(rhs.data_)} {}

  Buffer& operator=(Buffer&& rhs) noexcept {
    if (this != &rhs) {
      info_ = std::move(rhs.info_);
      data_ = std::move(rhs.data_);
    }
    return *this;
  }

  [[nodiscard]] bool operator==(const Buffer& other) const noexcept = default;
  [[nodiscard]] bool operator!=(const Buffer& other) const noexcept = default;

  ~Buffer() noexcept = default;

  // ── Allocate zeroed ──────────────────────────────────────────

  Buffer(std::size_t width, std::size_t height, image::Color color)
      : info_{.width  = ValidateNonZero(width, height),
              .height = height,
              .color  = color},
        data_(info_.ByteCount()) {}

  // ── Move vector in (no copy) ─────────────────────────────────

  Buffer(std::size_t width, std::size_t height, image::Color color,
         std::vector<std::uint8_t> data)
      : info_{.width  = ValidateNonZero(width, height),
              .height = height,
              .color  = color} {
    auto expected = info_.ByteCount();

    if (data.size() != expected) {
      throw BufferDataSizeMismatchError(expected, data.size());
    }
    data_ = std::move(data);
  }

  // ── deducing this: operator[] ────────────────────────────────

  auto operator[](this auto&& self, std::size_t index) noexcept(!Debug)
      -> decltype(std::span{self.Data(), self.ChannelCount()}) {
    if constexpr (Debug) {
      self.ValidateInvariantOrThrow();
      if (self.Empty()) {
        throw BufferNotInitializedError();
      }
      if (index >= self.PixelCount()) {
        throw BufferIndexOutOfRangeError(index, self.PixelCount());
      }
    }
    return std::span{self.Data() + (index * self.ChannelCount()),
                     self.ChannelCount()};
  }

  auto operator[](this auto&& self, std::size_t x,
                  std::size_t y) noexcept(!Debug)
      -> decltype(std::span{self.Data(), self.ChannelCount()}) {
    if constexpr (Debug) {
      self.ValidateInvariantOrThrow();
      if (self.Empty()) {
        throw BufferNotInitializedError();
      }
      if (y >= self.Height() || x >= self.Width()) {
        throw BufferIndexOutOfRangeError(y, x, self.Height(), self.Width());
      }
    }
    return std::forward<decltype(self)>(self)[y * self.Width() + x];
  }

  // ── deducing this: Data ──────────────────────────────────────

  [[nodiscard]]
  auto Data(this auto&& self) noexcept {
    if constexpr (Debug) {
      self.AssertInvariant();
    }
    using Self = std::remove_reference_t<decltype(self)>;
    if constexpr (std::is_const_v<Self>)
      return self.data_.empty() ? static_cast<const std::uint8_t*>(nullptr)
                                : self.data_.data();
    else
      return self.data_.empty() ? static_cast<std::uint8_t*>(nullptr)
                                : self.data_.data();
  }

  // ── Implicit byte-span conversion ────────────────────────────

  [[nodiscard]]
  operator std::span<std::uint8_t>() noexcept(!Debug) {
    if constexpr (Debug) {
      ValidateInvariantOrThrow();
    }
    return {Data(), ByteCount()};
  }

  [[nodiscard]]
  operator std::span<const std::uint8_t>() const noexcept(!Debug) {
    if constexpr (Debug) {
      ValidateInvariantOrThrow();
    }
    return {Data(), ByteCount()};
  }

  // ── Clone ────────────────────────────────────────────────────

  [[nodiscard]]
  Buffer Clone(this const Buffer& self) {
    return Buffer(self.info_, self.data_);
  }

  // ── Getters ──────────────────────────────────────────────────

  [[nodiscard]] std::size_t Width() const noexcept {
    if constexpr (Debug) AssertInvariant();
    return info_.width;
  }
  [[nodiscard]] std::size_t Height() const noexcept {
    if constexpr (Debug) AssertInvariant();
    return info_.height;
  }
  [[nodiscard]] std::size_t ChannelCount() const noexcept {
    if constexpr (Debug) AssertInvariant();
    return info_.ChannelCount();
  }
  [[nodiscard]] std::size_t PixelCount() const noexcept {
    if constexpr (Debug) AssertInvariant();
    return info_.PixelCount();
  }
  [[nodiscard]] std::size_t StrideBytes() const noexcept {
    if constexpr (Debug) AssertInvariant();
    return info_.StrideBytes();
  }
  [[nodiscard]] std::size_t ByteCount() const noexcept {
    if constexpr (Debug) AssertInvariant();
    return info_.ByteCount();
  }
  [[nodiscard]] image::Color Color() const noexcept {
    if constexpr (Debug) AssertInvariant();
    return info_.color;
  }
  [[nodiscard]] bool Empty() const noexcept { return data_.empty(); }

  [[nodiscard]] BufferInfo Info() const noexcept {
    if constexpr (Debug) AssertInvariant();
    return info_;
  }

 private:
  [[nodiscard]]
  static constexpr bool IsColorValueValid(image::Color color) noexcept {
    const auto value = static_cast<int>(color);
    return value >= 0 && value < static_cast<int>(ColorCount);
  }

  [[nodiscard]]
  static constexpr bool MulWouldOverflow(std::size_t a,
                                         std::size_t b) noexcept {
    return a != 0 && b > std::numeric_limits<std::size_t>::max() / a;
  }

  [[nodiscard]]
  std::size_t ExpectedByteCountOrThrow() const {
    return info_.ByteCount();
  }

  void ValidateInvariantOrThrow() const {
    if (data_.empty()) {
      return;
    }

    if (info_.width == 0 || info_.height == 0) {
      throw BufferInvariantError("Non-empty buffer has zero width or height");
    }
    if (!IsColorValueValid(info_.color)) {
      throw BufferInvariantError("Buffer color enum value is invalid");
    }
    const auto expected_byte_count = ExpectedByteCountOrThrow();
    if (data_.size() != expected_byte_count) {
      throw BufferInvariantError("Buffer byte size does not match metadata");
    }
  }

  void AssertInvariant() const noexcept {
    if constexpr (!Debug) {
      return;
    }

    if (data_.empty()) {
      return;
    }

    assert(info_.width > 0);
    assert(info_.height > 0);
    assert(IsColorValueValid(info_.color));
    if (!IsColorValueValid(info_.color)) {
      return;
    }
    const auto channel_count = color::ChannelCount(info_.color);
    assert(channel_count > 0);

    assert(!MulWouldOverflow(info_.width, channel_count));
    if (MulWouldOverflow(info_.width, channel_count)) {
      return;
    }
    const auto stride_bytes = info_.width * channel_count;

    assert(!MulWouldOverflow(info_.height, stride_bytes));
    if (MulWouldOverflow(info_.height, stride_bytes)) {
      return;
    }
    const auto expected = info_.height * stride_bytes;
    assert(data_.size() == expected);
  }

  [[nodiscard]]
  static constexpr std::size_t ValidateNonZero(std::size_t width,
                                               std::size_t height) {
    if (width == 0 || height == 0) {
      throw BufferInvalidSizeError(width, height);
    }
    return width;
  }

  [[nodiscard]]
  static constexpr std::size_t MulOrThrow(std::size_t a, std::size_t b) {
    if (a != 0 && b > std::numeric_limits<std::size_t>::max() / a) {
      throw BufferOverflowError(a, b);
    }
    return a * b;
  }

  BufferInfo info_;
  std::vector<std::uint8_t> data_;
};

}  // namespace weqeqq::image

// ── std::formatter ─────────────────────────────────────────────────

template <>
struct std::formatter<weqeqq::image::Buffer> {
  static constexpr std::size_t MaxRows = 8;
  static constexpr std::size_t MaxCols = 8;

  enum class Mode : std::uint8_t { Dec, Hex, Bin } mode = Mode::Dec;

  constexpr auto parse(std::format_parse_context& ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it == 'x') {
      mode = Mode::Hex;
      ++it;
    }
    if (it != ctx.end() && *it != '}')
      throw std::format_error(
          "Invalid format spec for Buffer: expected 'x' or empty");
    return it;
  }

  auto format(const weqeqq::image::Buffer& buffer,
              std::format_context& ctx) const {
    auto out = ctx.out();

    if (buffer.Empty()) return std::format_to(out, "Buffer(empty)");

    auto data     = std::span<const std::uint8_t>(buffer);
    auto width    = buffer.Width();
    auto height   = buffer.Height();
    auto channels = buffer.ChannelCount();

    auto rows           = std::min(height, MaxRows);
    auto cols           = std::min(width, MaxCols);
    bool rows_truncated = height > MaxRows;
    bool cols_truncated = width > MaxCols;

    out = std::format_to(out, "Buffer({}x{}, Color::{}) [\n", width, height,
                         buffer.Color());

    for (std::size_t y = 0; y < rows; ++y) {
      out = std::format_to(out, "  [");

      for (std::size_t x = 0; x < cols; ++x) {
        auto offset = (y * width + x) * channels;

        if (x > 0) out = std::format_to(out, ", ");
        out = std::format_to(out, "(");

        for (std::size_t c = 0; c < channels; ++c) {
          if (c > 0) out = std::format_to(out, ", ");
          out = (mode == Mode::Hex)
                    ? std::format_to(out, "{:02x}", data[offset + c])
                    : std::format_to(out, "{:3}", data[offset + c]);
        }

        out = std::format_to(out, ")");
      }

      if (cols_truncated) out = std::format_to(out, ", ...");
      out = std::format_to(out, "]\n");
    }

    if (rows_truncated) out = std::format_to(out, "  ...\n");

    return std::format_to(out, "]");
  }
};
