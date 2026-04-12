#include <gtest/gtest.h>
#include <weqeqq/image/buffer.h>

#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace weqeqq::image {
namespace {

std::size_t ByteCount(std::span<const std::uint8_t> bytes) {
  return bytes.size();
}

void SetFirstByte(std::span<std::uint8_t> bytes, std::uint8_t value) {
  ASSERT_FALSE(bytes.empty());
  bytes[0] = value;
}

TEST(BufferTest, ConstructZeroedBufferSetsMetadataAndSizes) {
  Buffer buffer(2, 3, Color::kRgba);

  EXPECT_EQ(buffer.Width(), 2u);
  EXPECT_EQ(buffer.Height(), 3u);
  EXPECT_EQ(buffer.ChannelCount(), 4u);
  EXPECT_EQ(buffer.PixelCount(), 6u);
  EXPECT_EQ(buffer.StrideBytes(), 8u);
  EXPECT_EQ(buffer.ByteCount(), 24u);
  EXPECT_EQ(buffer.Info().StrideBytes(), 8u);
  EXPECT_EQ(buffer.Info().ByteCount(), 24u);
  EXPECT_FALSE(buffer.Empty());
}

TEST(BufferTest, ConstructFromDataVectorAcceptsExactSize) {
  std::vector<std::uint8_t> data(2 * 2 * 4, 7);
  Buffer buffer(2, 2, Color::kRgba, std::move(data));

  EXPECT_EQ(buffer.PixelCount(), 4u);
  EXPECT_EQ(buffer.ByteCount(), 16u);
  EXPECT_EQ(buffer[0][0], 7);
}

TEST(BufferTest, ConstructRejectsZeroDimensions) {
  EXPECT_THROW((Buffer(0, 1, Color::kRgba)), BufferInvalidDimensionsError);
  EXPECT_THROW((Buffer(1, 0, Color::kRgba)), BufferInvalidDimensionsError);
}

TEST(BufferTest, InvalidSizeErrorExposesStructuredMetadata) {
  try {
    (void)Buffer(0, 1, Color::kRgba);
    FAIL() << "Expected BufferInvalidDimensionsError";
  } catch (const BufferInvalidDimensionsError &error) {
    EXPECT_EQ(error.TypedCode(), ErrorCode::kBufferInvalidDimensions);
    ASSERT_TRUE(error.HasCode());
    EXPECT_EQ(error.Code(),
              make_error_code(ErrorCode::kBufferInvalidDimensions));

    const auto *width  = error.FindField("width");
    const auto *height = error.FindField("height");
    ASSERT_NE(width, nullptr);
    ASSERT_NE(height, nullptr);
    EXPECT_EQ(width->value, "0");
    EXPECT_EQ(height->value, "1");
    EXPECT_FALSE(error.Details().empty());
    EXPECT_FALSE(error.Hint().empty());
  } catch (...) {
    FAIL() << "Expected BufferInvalidDimensionsError";
  }
}

TEST(BufferInfoTest, PixelCountOverflowThrowsSpecificError) {
  BufferInfo info{
      .width  = std::numeric_limits<std::size_t>::max(),
      .height = 2,
      .color  = Color::kRgba,
  };

  EXPECT_THROW((void)info.PixelCount(), BufferPixelCountOverflowError);
}

TEST(BufferInfoTest, StrideOverflowThrowsSpecificError) {
  BufferInfo info{
      .width  = std::numeric_limits<std::size_t>::max() / 4 + 1,
      .height = 1,
      .color  = Color::kRgba,
  };

  EXPECT_THROW((void)info.StrideBytes(), BufferStrideOverflowError);
}

TEST(BufferInfoTest, ByteCountOverflowThrowsSpecificError) {
  BufferInfo info{
      .width  = std::numeric_limits<std::size_t>::max() / 4,
      .height = 2,
      .color  = Color::kRgba,
  };

  EXPECT_THROW((void)info.ByteCount(), BufferByteCountOverflowError);
}

TEST(BufferTest, ConstructRejectsMismatchedDataSize) {
  EXPECT_THROW((Buffer(2, 2, Color::kRgba, std::vector<std::uint8_t>(15))),
               BufferDataSizeMismatchError);
}

TEST(BufferTest, FlatIndexReturnsPixelSpan) {
  Buffer buffer(2, 1, Color::kRgba, {1, 2, 3, 4, 5, 6, 7, 8});

  auto pixel = buffer[1];
  EXPECT_EQ(pixel.size(), 4u);
  EXPECT_EQ(pixel[0], 5);
  EXPECT_EQ(pixel[3], 8);
}

TEST(BufferTest, CoordinateIndexReturnsPixelSpan) {
  Buffer buffer(2, 2, Color::kRgba,
                {
                    1,
                    2,
                    3,
                    4,
                    5,
                    6,
                    7,
                    8,
                    9,
                    10,
                    11,
                    12,
                    13,
                    14,
                    15,
                    16,
                });

  auto pixel = buffer[1, 1];
  EXPECT_EQ(pixel[0], 13);
  EXPECT_EQ(pixel[3], 16);
}

TEST(BufferTest, ImplicitMutableSpanConversionWorks) {
  Buffer buffer(1, 1, Color::kRgba, {10, 20, 30, 40});

  SetFirstByte(buffer, 99);

  EXPECT_EQ(buffer[0][0], 99);
}

TEST(BufferTest, ImplicitConstSpanConversionWorks) {
  const Buffer buffer(1, 1, Color::kRgba, {10, 20, 30, 40});

  EXPECT_EQ(ByteCount(buffer), buffer.ByteCount());
}

TEST(BufferTest, SpanConversionSizeEqualsByteCount) {
  Buffer buffer(3, 2, Color::kRgb);
  std::span<std::uint8_t> bytes = buffer;

  EXPECT_EQ(bytes.size(), buffer.ByteCount());
}

TEST(BufferTest, EmptyBufferConvertsToEmptySpan) {
  Buffer buffer;

  std::span<std::uint8_t> bytes = buffer;
  EXPECT_EQ(bytes.size(), 0u);
  EXPECT_EQ(bytes.data(), nullptr);
}

TEST(BufferTest, UninitializedBufferIndexThrowsInDebug) {
  if constexpr (!Buffer::Debug) {
    GTEST_SKIP() << "Debug-only behavior";
  }

  Buffer buffer;
  EXPECT_THROW((void)buffer[0], BufferUninitializedError);
}

TEST(BufferTest, FlatIndexOutOfRangeThrowsInDebug) {
  if constexpr (!Buffer::Debug) {
    GTEST_SKIP() << "Debug-only behavior";
  }

  Buffer buffer(1, 1, Color::kRgba);
  EXPECT_THROW((void)buffer[1], BufferLinearIndexOutOfBoundsError);
}

TEST(BufferTest, CoordinateOutOfRangeThrowsInDebug) {
  if constexpr (!Buffer::Debug) {
    GTEST_SKIP() << "Debug-only behavior";
  }

  Buffer buffer(1, 1, Color::kRgba);
  EXPECT_THROW(([&] { (void)buffer[1, 0]; }()),
               BufferCoordinateOutOfBoundsError);
  EXPECT_THROW(([&] { (void)buffer[0, 1]; }()),
               BufferCoordinateOutOfBoundsError);
}

struct BufferLayoutMirror {
  BufferInfo info_;
  std::vector<std::uint8_t> data_;
};

static_assert(sizeof(BufferLayoutMirror) == sizeof(Buffer));
static_assert(alignof(BufferLayoutMirror) == alignof(Buffer));

BufferLayoutMirror &Raw(Buffer &buffer) {
  return reinterpret_cast<BufferLayoutMirror &>(buffer);
}

TEST(BufferTest, DebugInvariantThrowsForZeroDimensionsWithData) {
  if constexpr (!Buffer::Debug) {
    GTEST_SKIP() << "Debug-only behavior";
  }

  Buffer buffer(1, 1, Color::kRgba, {1, 2, 3, 4});
  auto &raw       = Raw(buffer);
  raw.info_.width = 0;

  EXPECT_THROW((void)static_cast<std::span<std::uint8_t>>(buffer),
               BufferZeroDimensionsInvariantError);
}

TEST(BufferTest, DebugInvariantThrowsForInvalidColorValue) {
  if constexpr (!Buffer::Debug) {
    GTEST_SKIP() << "Debug-only behavior";
  }

  Buffer buffer(1, 1, Color::kRgba, {1, 2, 3, 4});
  auto &raw       = Raw(buffer);
  raw.info_.color = static_cast<Color>(255);

  EXPECT_THROW((void)buffer[0], BufferInvalidColorInvariantError);
}

TEST(BufferTest, DebugInvariantThrowsForByteCountMismatch) {
  if constexpr (!Buffer::Debug) {
    GTEST_SKIP() << "Debug-only behavior";
  }

  Buffer buffer(1, 1, Color::kRgba, {1, 2, 3, 4});
  auto &raw = Raw(buffer);
  raw.data_.push_back(5);

  EXPECT_THROW((void)static_cast<std::span<const std::uint8_t>>(buffer),
               BufferByteCountInvariantError);
}

}  // namespace
}  // namespace weqeqq::image
