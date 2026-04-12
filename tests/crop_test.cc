#include <gtest/gtest.h>
#include <weqeqq/image/crop.h>

#include <cstdint>
#include <vector>

namespace weqeqq::image {
namespace {

Buffer MakePatternBuffer(std::size_t width, std::size_t height, Color color) {
  Buffer buffer(width, height, color);

  for (std::size_t y = 0; y < height; ++y) {
    for (std::size_t x = 0; x < width; ++x) {
      for (std::size_t channel = 0; channel < buffer.ChannelCount();
           ++channel) {
        buffer[x, y][channel] =
            static_cast<std::uint8_t>(y * 40 + x * 7 + channel * 3 + 1);
      }
    }
  }

  return buffer;
}

TEST(CropTest, ReturnsRequestedInteriorRegion) {
  const auto input  = MakePatternBuffer(4, 3, Color::kRgba);
  const auto actual = Crop(input, 1, 1, 2, 2);

  EXPECT_EQ(actual.Width(), 2u);
  EXPECT_EQ(actual.Height(), 2u);
  EXPECT_EQ(actual.Color(), Color::kRgba);
  EXPECT_EQ(actual.ChannelCount(), 4u);

  for (std::size_t y = 0; y < actual.Height(); ++y) {
    for (std::size_t x = 0; x < actual.Width(); ++x) {
      const auto actual_pixel = actual[x, y];
      const auto input_pixel  = input[x + 1, y + 1];
      for (std::size_t channel = 0; channel < actual.ChannelCount();
           ++channel) {
        EXPECT_EQ(actual_pixel[channel], input_pixel[channel])
            << "x=" << x << " y=" << y << " channel=" << channel;
      }
    }
  }
}

TEST(CropTest, CroppingWholeImageMatchesClone) {
  const auto input  = MakePatternBuffer(3, 2, Color::kRgba);
  const auto actual = Crop(input, 0, 0, input.Width(), input.Height());
  const auto clone  = input.Clone();

  EXPECT_EQ(actual, clone);
}

TEST(CropTest, CroppingBottomRightRegionKeepsCorrectLayout) {
  const auto input  = MakePatternBuffer(5, 4, Color::kRgba);
  const auto actual = Crop(input, 3, 2, 2, 2);

  for (std::size_t y = 0; y < 2; ++y) {
    for (std::size_t x = 0; x < 2; ++x) {
      const auto actual_pixel = actual[x, y];
      const auto input_pixel  = input[x + 3, y + 2];
      for (std::size_t channel = 0; channel < actual.ChannelCount();
           ++channel) {
        EXPECT_EQ(actual_pixel[channel], input_pixel[channel])
            << "x=" << x << " y=" << y << " channel=" << channel;
      }
    }
  }
}

TEST(CropTest, PreservesMetadataForRgbBuffers) {
  const auto input  = MakePatternBuffer(4, 3, Color::kRgb);
  const auto actual = Crop(input, 1, 0, 3, 2);

  EXPECT_EQ(actual.Width(), 3u);
  EXPECT_EQ(actual.Height(), 2u);
  EXPECT_EQ(actual.Color(), Color::kRgb);
  EXPECT_EQ(actual.ChannelCount(), 3u);
  EXPECT_EQ(actual.StrideBytes(), 9u);
  EXPECT_EQ(actual.ByteCount(), 18u);
}

TEST(CropTest, RejectsZeroCropDimensions) {
  const auto input = MakePatternBuffer(2, 2, Color::kRgba);

  EXPECT_THROW((void)Crop(input, 0, 0, 0, 1), CropInvalidSizeError);
  EXPECT_THROW((void)Crop(input, 0, 0, 1, 0), CropInvalidSizeError);
}

TEST(CropTest, RejectsOutOfBoundsRegion) {
  const auto input = MakePatternBuffer(3, 3, Color::kRgba);

  EXPECT_THROW((void)Crop(input, 3, 0, 1, 1), CropRegionOutOfBoundsError);
  EXPECT_THROW((void)Crop(input, 0, 3, 1, 1), CropRegionOutOfBoundsError);
  EXPECT_THROW((void)Crop(input, 2, 1, 2, 1), CropRegionOutOfBoundsError);
  EXPECT_THROW((void)Crop(input, 1, 2, 1, 2), CropRegionOutOfBoundsError);
}

}  // namespace
}  // namespace weqeqq::image
