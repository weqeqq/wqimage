
#include <gtest/gtest.h>

//
#include <weqeqq/image/blending.h>

#include <cstdint>
#include <vector>

namespace weqeqq::image {

inline Buffer CreateBuffer(std::vector<std::uint8_t> data) {
  return Buffer(3, 3, Color::kRgba, std::move(data));
}

/* clang-format off */

struct DataForBlendingTest {
  inline static const auto Source = CreateBuffer(
    {
      0x20, 0x20, 0x20, 0xff,   0x20, 0x20, 0x20, 0xff,   0x20, 0x20, 0x20, 0xff,
      0x80, 0x80, 0x80, 0xff,   0x80, 0x80, 0x80, 0xff,   0x80, 0x80, 0x80, 0xff,
      0xe0, 0xe0, 0xe0, 0xff,   0xe0, 0xe0, 0xe0, 0xff,   0xe0, 0xe0, 0xe0, 0xff
    });
  inline static const auto Destination = CreateBuffer(
    {
      0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff,
      0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff,
      0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff
    });
  template <Blending B>
  inline static const auto ExpectedOutputFor = CreateBuffer({});
};

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kNormal> = CreateBuffer({
  0x20, 0x20, 0x20, 0xff,   0x20, 0x20, 0x20, 0xff,   0x20, 0x20, 0x20, 0xff,
  0x80, 0x80, 0x80, 0xff,   0x80, 0x80, 0x80, 0xff,   0x80, 0x80, 0x80, 0xff,
  0xe0, 0xe0, 0xe0, 0xff,   0xe0, 0xe0, 0xe0, 0xff,   0xe0, 0xe0, 0xe0, 0xff
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kDarken> = CreateBuffer({
  0x20, 0x20, 0x20, 0xff,   0x20, 0x20, 0x20, 0xff,   0x20, 0x20, 0x20, 0xff,
  0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0x80, 0x80, 0x80, 0xff,
  0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kMultiply> = CreateBuffer({
  0x08, 0x08, 0x08, 0xff,   0x10, 0x10, 0x10, 0xff,   0x18, 0x18, 0x18, 0xff,
  0x20, 0x20, 0x20, 0xff,   0x40, 0x40, 0x40, 0xff,   0x60, 0x60, 0x60, 0xff,
  0x38, 0x38, 0x38, 0xff,   0x70, 0x70, 0x70, 0xff,   0xa9, 0xa9, 0xa9, 0xff,
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kColorBurn> = CreateBuffer({
  0x00, 0x00, 0x00, 0xff,   0x00, 0x00, 0x00, 0xff,   0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff,   0x02, 0x02, 0x02, 0xff,   0x82, 0x82, 0x82, 0xff,
  0x26, 0x26, 0x26, 0xff,   0x6f, 0x6f, 0x6f, 0xff,   0xb8, 0xb8, 0xb8, 0xff,
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kLinearBurn> = CreateBuffer({
  0x00, 0x00, 0x00, 0xff,   0x00, 0x00, 0x00, 0xff,   0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff,   0x01, 0x01, 0x01, 0xff,   0x41, 0x41, 0x41, 0xff,
  0x21, 0x21, 0x21, 0xff,   0x61, 0x61, 0x61, 0xff,   0xa1, 0xa1, 0xa1, 0xff,
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kLighten> = CreateBuffer({
  0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff,
  0x80, 0x80, 0x80, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff,
  0xe0, 0xe0, 0xe0, 0xff,   0xe0, 0xe0, 0xe0, 0xff,   0xe0, 0xe0, 0xe0, 0xff,
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kScreen> = CreateBuffer({
  0x58, 0x58, 0x58, 0xff,   0x90, 0x90, 0x90, 0xff,   0xc8, 0xc8, 0xc8, 0xff,
  0xa0, 0xa0, 0xa0, 0xff,   0xc0, 0xc0, 0xc0, 0xff,   0xe0, 0xe0, 0xe0, 0xff,
  0xe8, 0xe8, 0xe8, 0xff,   0xf0, 0xf0, 0xf0, 0xff,   0xf7, 0xf7, 0xf7, 0xff,
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kColorDodge> = CreateBuffer({
  0x49, 0x49, 0x49, 0xff,   0x92, 0x92, 0x92, 0xff,   0xdb, 0xdb, 0xdb, 0xff,
  0x80, 0x80, 0x80, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kLinearDodge> = CreateBuffer({
  0x60, 0x60, 0x60, 0xff,   0xa0, 0xa0, 0xa0, 0xff,   0xe0, 0xe0, 0xe0, 0xff,
  0xc0, 0xc0, 0xc0, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kOverlay> = CreateBuffer({
  0x10, 0x10, 0x10, 0xff,   0x21, 0x21, 0x21, 0xff,   0x91, 0x91, 0x91, 0xff,
  0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff,
  0x70, 0x70, 0x70, 0xff,   0xe0, 0xe0, 0xe0, 0xff,   0xf0, 0xf0, 0xf0, 0xff,
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kSoftLight> = CreateBuffer({
  0x1d, 0x1d, 0x1d, 0xff,   0x51, 0x51, 0x51, 0xff,   0x9d, 0x9d, 0x9d, 0xff,
  0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff,
  0x6f, 0x6f, 0x6f, 0xff,   0xa7, 0xa7, 0xa7, 0xff,   0xd5, 0xd5, 0xd5, 0xff,
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kHardLight> = CreateBuffer({
  0x10, 0x10, 0x10, 0xff,   0x20, 0x20, 0x20, 0xff,   0x30, 0x30, 0x30, 0xff,
  0x41, 0x41, 0x41, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff,
  0xd1, 0xd1, 0xd1, 0xff,   0xe0, 0xe0, 0xe0, 0xff,   0xf0, 0xf0, 0xf0, 0xff,
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kVividLight> = CreateBuffer({
  0x00, 0x00, 0x00, 0xff,   0x00, 0x00, 0x00, 0xff,   0x04, 0x04, 0x04, 0xff,
  0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff,
  0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kLinearLight> = CreateBuffer({
  0x00, 0x00, 0x00, 0xff,   0x00, 0x00, 0x00, 0xff,   0x01, 0x01, 0x01, 0xff,
  0x41, 0x41, 0x41, 0xff,   0x81, 0x81, 0x81, 0xff,   0xc1, 0xc1, 0xc1, 0xff,
  0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kPinLight> = CreateBuffer({
  0x40, 0x40, 0x40, 0xff,   0x40, 0x40, 0x40, 0xff,   0x40, 0x40, 0x40, 0xff,
  0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff,
  0xc1, 0xc1, 0xc1, 0xff,   0xc1, 0xc1, 0xc1, 0xff,   0xc1, 0xc1, 0xc1, 0xff,
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kHardMix> = CreateBuffer({
  0x00, 0x00, 0x00, 0xff,   0x00, 0x00, 0x00, 0xff,   0x00, 0x00, 0x00, 0xff,
  0x00, 0x00, 0x00, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kDifference> = CreateBuffer({
  0x20, 0x20, 0x20, 0xff,   0x60, 0x60, 0x60, 0xff,   0xa0, 0xa0, 0xa0, 0xff,
  0x40, 0x40, 0x40, 0xff,   0x00, 0x00, 0x00, 0xff,   0x40, 0x40, 0x40, 0xff,
  0xa0, 0xa0, 0xa0, 0xff,   0x60, 0x60, 0x60, 0xff,   0x20, 0x20, 0x20, 0xff,
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kExclusion> = CreateBuffer({
  0x50, 0x50, 0x50, 0xff,   0x80, 0x80, 0x80, 0xff,   0xb0, 0xb0, 0xb0, 0xff,
  0x80, 0x80, 0x80, 0xff,   0x7f, 0x7f, 0x7f, 0xff,   0x7f, 0x7f, 0x7f, 0xff,
  0xb0, 0xb0, 0xb0, 0xff,   0x7f, 0x7f, 0x7f, 0xff,   0x4f, 0x4f, 0x4f, 0xff
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kSubtract> = CreateBuffer({
  0x20, 0x20, 0x20, 0xff,   0x60, 0x60, 0x60, 0xff,   0xa0, 0xa0, 0xa0, 0xff,
  0x00, 0x00, 0x00, 0xff,   0x00, 0x00, 0x00, 0xff,   0x40, 0x40, 0x40, 0xff,
  0x00, 0x00, 0x00, 0xff,   0x00, 0x00, 0x00, 0xff,   0x00, 0x00, 0x00, 0xff,
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kDivide> = CreateBuffer({
  0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,
  0x7f, 0x7f, 0x7f, 0xff,   0xff, 0xff, 0xff, 0xff,   0xff, 0xff, 0xff, 0xff,
  0x48, 0x48, 0x48, 0xff,   0x91, 0x91, 0x91, 0xff,   0xda, 0xda, 0xda, 0xff,
});

// kHue, kSaturation, kColor: source and destination are both grey (Sat=0),
// so the result is always the destination (no hue/sat to apply).
template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kHue> = CreateBuffer({
  0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff,
  0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff,
  0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff,
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kSaturation> = CreateBuffer({
  0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff,
  0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff,
  0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff,
});

template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kColor> = CreateBuffer({
  0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff,
  0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff,
  0x40, 0x40, 0x40, 0xff,   0x80, 0x80, 0x80, 0xff,   0xc0, 0xc0, 0xc0, 0xff,
});

// kLuminosity: luminosity of source replaces destination (both grey, so result = source).
template <>
inline const auto DataForBlendingTest::ExpectedOutputFor<Blending::kLuminosity> = CreateBuffer({
  0x20, 0x20, 0x20, 0xff,   0x20, 0x20, 0x20, 0xff,   0x20, 0x20, 0x20, 0xff,
  0x80, 0x80, 0x80, 0xff,   0x80, 0x80, 0x80, 0xff,   0x80, 0x80, 0x80, 0xff,
  0xe0, 0xe0, 0xe0, 0xff,   0xe0, 0xe0, 0xe0, 0xff,   0xe0, 0xe0, 0xe0, 0xff,
});

/* clang-format on */

template <typename T>
class BlendingTest : public ::testing::Test {};

template <Blending B>
struct BlendingConstant {
  static constexpr Blending Value = B;
};

using BlendingTypes = ::testing::Types<
    BlendingConstant<Blending::kNormal>, BlendingConstant<Blending::kDarken>,
    BlendingConstant<Blending::kMultiply>,
    BlendingConstant<Blending::kColorBurn>,
    BlendingConstant<Blending::kLinearBurn>,
    BlendingConstant<Blending::kLighten>, BlendingConstant<Blending::kScreen>,
    BlendingConstant<Blending::kColorDodge>,
    BlendingConstant<Blending::kLinearDodge>,
    BlendingConstant<Blending::kOverlay>,
    BlendingConstant<Blending::kSoftLight>,
    BlendingConstant<Blending::kHardLight>,
    BlendingConstant<Blending::kVividLight>,
    BlendingConstant<Blending::kLinearLight>,
    BlendingConstant<Blending::kPinLight>, BlendingConstant<Blending::kHardMix>,
    BlendingConstant<Blending::kDifference>,
    BlendingConstant<Blending::kExclusion>,
    BlendingConstant<Blending::kSubtract>, BlendingConstant<Blending::kDivide>,
    BlendingConstant<Blending::kHue>, BlendingConstant<Blending::kSaturation>,
    BlendingConstant<Blending::kColor>,
    BlendingConstant<Blending::kLuminosity>>;

TYPED_TEST_SUITE(BlendingTest, BlendingTypes);

void Compare(const Buffer &actual, const Buffer &expected) {
  ASSERT_EQ(actual.Width(), expected.Width());
  ASSERT_EQ(actual.Height(), expected.Height());

  for (std::size_t index = 0; index < actual.PixelCount(); ++index) {
    EXPECT_EQ(actual[index][0], expected[index][0])
        << "pixel " << index << " channel R";
    EXPECT_EQ(actual[index][1], expected[index][1])
        << "pixel " << index << " channel G";
    EXPECT_EQ(actual[index][2], expected[index][2])
        << "pixel " << index << " channel B";
    EXPECT_EQ(actual[index][3], expected[index][3])
        << "pixel " << index << " channel A";
  }
}

void CompareWithTolerance(const Buffer &actual, const Buffer &expected,
                          std::uint8_t tolerance) {
  ASSERT_EQ(actual.Width(), expected.Width());
  ASSERT_EQ(actual.Height(), expected.Height());

  for (std::size_t index = 0; index < actual.PixelCount(); ++index) {
    for (std::size_t channel = 0; channel < actual.ChannelCount(); ++channel) {
      const auto a = actual[index][channel];
      const auto e = expected[index][channel];
      const auto d = (a > e) ? (a - e) : (e - a);
      EXPECT_LE(d, tolerance) << "pixel " << index << " channel " << channel
                              << " actual=" << static_cast<int>(a)
                              << " expected=" << static_cast<int>(e);
    }
  }
}

Buffer MakeUniformBuffer(std::size_t width, std::size_t height, std::uint8_t r,
                         std::uint8_t g, std::uint8_t b, std::uint8_t a) {
  std::vector<std::uint8_t> data(width * height * 4);
  for (std::size_t i = 0; i < width * height; ++i) {
    data[i * 4 + 0] = r;
    data[i * 4 + 1] = g;
    data[i * 4 + 2] = b;
    data[i * 4 + 3] = a;
  }
  return Buffer(width, height, Color::kRgba, std::move(data));
}

Buffer MakeUniformRgbBuffer(std::size_t width, std::size_t height,
                            std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  std::vector<std::uint8_t> data(width * height * 3);
  for (std::size_t i = 0; i < width * height; ++i) {
    data[i * 3 + 0] = r;
    data[i * 3 + 1] = g;
    data[i * 3 + 2] = b;
  }
  return Buffer(width, height, Color::kRgb, std::move(data));
}

Buffer MakeUniformGrayBuffer(std::size_t width, std::size_t height,
                             std::uint8_t value) {
  return Buffer(width, height, Color::kGrayscale,
                std::vector<std::uint8_t>(width * height, value));
}

std::size_t CountChangedPixels(const Buffer &actual, const Buffer &baseline) {
  std::size_t changed = 0;
  for (std::size_t index = 0; index < actual.PixelCount(); ++index) {
    bool differs = false;
    for (std::size_t channel = 0; channel < actual.ChannelCount(); ++channel) {
      if (actual[index][channel] != baseline[index][channel]) {
        differs = true;
        break;
      }
    }
    changed += differs ? 1u : 0u;
  }
  return changed;
}

TYPED_TEST(BlendingTest, OpaqueSourceOpaqueDestStraightAlpha) {
  constexpr auto kMode = TypeParam::Value;

  auto source      = DataForBlendingTest::Source.Clone();
  auto destination = DataForBlendingTest::Destination.Clone();
  auto expected    = DataForBlendingTest::ExpectedOutputFor<kMode>.Clone();

  BlendInPlace(destination, source, kMode, Alpha::kStraight);

  Compare(destination, expected);
}

TEST(BlendApiTest, NoOverlapLeavesDestinationUnchanged) {
  auto source      = MakeUniformBuffer(2, 2, 255, 0, 0, 255);
  auto destination = MakeUniformBuffer(3, 3, 10, 20, 30, 40);
  auto expected    = destination.Clone();

  BlendInPlace(destination, source, 5, 5, Blending::kNormal, Alpha::kStraight);
  Compare(destination, expected);

  BlendInPlace(destination, source, -10, -10, Blending::kNormal,
               Alpha::kStraight);
  Compare(destination, expected);
}

TEST(BlendApiTest, PositiveOffsetBlendsOnlyIntersectingRegion) {
  Buffer destination(4, 4, Color::kRgba, std::vector<std::uint8_t>(4 * 4 * 4));
  for (std::size_t i = 0; i < destination.PixelCount(); ++i) {
    destination[i][0] = static_cast<std::uint8_t>(i);
    destination[i][1] = static_cast<std::uint8_t>(i + 1);
    destination[i][2] = static_cast<std::uint8_t>(i + 2);
    destination[i][3] = 255;
  }
  auto expected = destination.Clone();

  auto source = MakeUniformBuffer(2, 2, 200, 100, 50, 255);
  BlendInPlace(destination, source, 1, 2, Blending::kNormal, Alpha::kStraight);

  for (std::size_t y = 0; y < 4; ++y) {
    for (std::size_t x = 0; x < 4; ++x) {
      const bool in_region = (x >= 1 && x < 3 && y >= 2 && y < 4);
      const auto idx       = y * 4 + x;
      if (in_region) {
        EXPECT_EQ(destination[idx][0], 200) << "x=" << x << " y=" << y;
        EXPECT_EQ(destination[idx][1], 100) << "x=" << x << " y=" << y;
        EXPECT_EQ(destination[idx][2], 50) << "x=" << x << " y=" << y;
        EXPECT_EQ(destination[idx][3], 255) << "x=" << x << " y=" << y;
      } else {
        EXPECT_EQ(destination[idx][0], expected[idx][0])
            << "x=" << x << " y=" << y;
        EXPECT_EQ(destination[idx][1], expected[idx][1])
            << "x=" << x << " y=" << y;
        EXPECT_EQ(destination[idx][2], expected[idx][2])
            << "x=" << x << " y=" << y;
        EXPECT_EQ(destination[idx][3], expected[idx][3])
            << "x=" << x << " y=" << y;
      }
    }
  }
}

TEST(BlendApiTest, NegativeOffsetCropsSourceCorrectly) {
  auto destination = MakeUniformBuffer(3, 3, 1, 2, 3, 255);
  auto expected    = destination.Clone();

  Buffer source(3, 3, Color::kRgba, std::vector<std::uint8_t>(3 * 3 * 4));
  for (std::size_t y = 0; y < 3; ++y) {
    for (std::size_t x = 0; x < 3; ++x) {
      const auto i = y * 3 + x;
      source[i][0] = static_cast<std::uint8_t>(10 * x + y);
      source[i][1] = static_cast<std::uint8_t>(100 + 10 * x + y);
      source[i][2] = static_cast<std::uint8_t>(200 + 10 * x + y);
      source[i][3] = 255;
    }
  }

  BlendInPlace(destination, source, -1, -1, Blending::kNormal,
               Alpha::kStraight);

  for (std::size_t y = 0; y < 3; ++y) {
    for (std::size_t x = 0; x < 3; ++x) {
      const auto idx = y * 3 + x;
      if (x < 2 && y < 2) {
        const auto src_idx = (y + 1) * 3 + (x + 1);
        EXPECT_EQ(destination[idx][0], source[src_idx][0])
            << "x=" << x << " y=" << y;
        EXPECT_EQ(destination[idx][1], source[src_idx][1])
            << "x=" << x << " y=" << y;
        EXPECT_EQ(destination[idx][2], source[src_idx][2])
            << "x=" << x << " y=" << y;
        EXPECT_EQ(destination[idx][3], 255) << "x=" << x << " y=" << y;
      } else {
        EXPECT_EQ(destination[idx][0], expected[idx][0])
            << "x=" << x << " y=" << y;
        EXPECT_EQ(destination[idx][1], expected[idx][1])
            << "x=" << x << " y=" << y;
        EXPECT_EQ(destination[idx][2], expected[idx][2])
            << "x=" << x << " y=" << y;
        EXPECT_EQ(destination[idx][3], expected[idx][3])
            << "x=" << x << " y=" << y;
      }
    }
  }
}

TEST(BlendApiTest, PremultipliedMatchesStraightForNormalMode) {
  Buffer source(2, 2, Color::kRgba,
                {
                    250,
                    20,
                    10,
                    128,
                    10,
                    250,
                    20,
                    64,
                    20,
                    10,
                    250,
                    192,
                    240,
                    240,
                    40,
                    32,
                });
  Buffer destination(2, 2, Color::kRgba,
                     {
                         10,
                         20,
                         30,
                         255,
                         200,
                         30,
                         40,
                         180,
                         50,
                         220,
                         60,
                         90,
                         70,
                         80,
                         240,
                         140,
                     });

  auto straight_dst = destination.Clone();
  BlendInPlace(straight_dst, source, Blending::kNormal, Alpha::kStraight);

  auto premul_src = Premultiply(source.Clone());
  auto premul_dst = Premultiply(destination.Clone());
  BlendInPlace(premul_dst, premul_src, Blending::kNormal,
               Alpha::kPremultiplied);
  UnpremultiplyInPlace(premul_dst);

  CompareWithTolerance(premul_dst, straight_dst, 1);
}

TEST(BlendOpacityTest, ZeroOpacityLeavesDestinationUnchangedForMultiply) {
  auto source         = MakeUniformBuffer(2, 2, 200, 100, 50, 255);
  auto destination    = MakeUniformBuffer(2, 2, 40, 80, 120, 255);
  const auto expected = destination.Clone();

  BlendInPlace(destination, source, Blending::kMultiply, Alpha::kStraight, 0);

  EXPECT_EQ(destination, expected);
}

TEST(BlendOpacityTest, ZeroOpacityLeavesDestinationUnchangedForHue) {
  Buffer source(1, 1, Color::kRgba, {255, 0, 0, 255});
  Buffer destination(1, 1, Color::kRgba, {0, 0, 255, 255});
  const auto expected = destination.Clone();

  BlendInPlace(destination, source, Blending::kHue, Alpha::kStraight, 0);

  EXPECT_EQ(destination, expected);
}

TEST(BlendOpacityTest, RgbaNormalHalfOpacityMixesWithDestination) {
  Buffer source(1, 1, Color::kRgba, {220, 140, 100, 255});
  Buffer destination(1, 1, Color::kRgba, {20, 40, 60, 255});

  BlendInPlace(destination, source, Blending::kNormal, Alpha::kStraight, 50);

  EXPECT_EQ(destination[0][0], 120);
  EXPECT_EQ(destination[0][1], 90);
  EXPECT_EQ(destination[0][2], 80);
  EXPECT_EQ(destination[0][3], 255);
}

TEST(BlendOpacityTest, RgbNormalHalfOpacityLerpsFromDestinationToBlendResult) {
  auto source         = MakeUniformRgbBuffer(1, 1, 200, 100, 0);
  auto destination    = MakeUniformRgbBuffer(1, 1, 0, 100, 200);
  const auto expected = MakeUniformRgbBuffer(1, 1, 100, 100, 100);

  BlendInPlace(destination, source, Blending::kNormal, Alpha::kStraight, 50);

  EXPECT_EQ(destination, expected);
}

TEST(BlendOpacityTest,
     GrayscaleNormalHalfOpacityLerpsFromDestinationToBlendResult) {
  Buffer source(3, 1, Color::kGrayscale, {220, 120, 20});
  auto destination  = MakeUniformGrayBuffer(3, 1, 20);
  destination[1][0] = 120;
  destination[2][0] = 220;
  const Buffer expected(3, 1, Color::kGrayscale, {120, 120, 120});

  BlendInPlace(destination, source, Blending::kNormal, Alpha::kStraight, 50);

  EXPECT_EQ(destination, expected);
}

TEST(BlendOpacityTest, PremultipliedMatchesStraightForNormalMode) {
  Buffer source(2, 2, Color::kRgba,
                {
                    250,
                    20,
                    10,
                    128,
                    10,
                    250,
                    20,
                    64,
                    20,
                    10,
                    250,
                    192,
                    240,
                    240,
                    40,
                    32,
                });
  Buffer destination(2, 2, Color::kRgba,
                     {
                         10,
                         20,
                         30,
                         255,
                         200,
                         30,
                         40,
                         180,
                         50,
                         220,
                         60,
                         90,
                         70,
                         80,
                         240,
                         140,
                     });

  auto straight_dst = destination.Clone();
  BlendInPlace(straight_dst, source, Blending::kNormal, Alpha::kStraight, 50);

  auto premul_src = Premultiply(source.Clone());
  auto premul_dst = Premultiply(destination.Clone());
  BlendInPlace(premul_dst, premul_src, Blending::kNormal, Alpha::kPremultiplied,
               50);
  UnpremultiplyInPlace(premul_dst);

  CompareWithTolerance(premul_dst, straight_dst, 1);
}

TEST(BlendOpacityTest, OpacityOutOfRangeErrorExposesStructuredMetadata) {
  auto source      = MakeUniformBuffer(1, 1, 255, 0, 0, 255);
  auto destination = MakeUniformBuffer(1, 1, 0, 0, 255, 255);

  try {
    BlendInPlace(destination, source, Blending::kNormal, Alpha::kStraight, 101);
    FAIL() << "Expected BlendOpacityOutOfRangeError";
  } catch (const BlendOpacityOutOfRangeError &error) {
    EXPECT_EQ(error.TypedCode(), ErrorCode::kBlendOpacityOutOfRange);

    const auto *value = error.FindField("value");
    const auto *max   = error.FindField("max");
    ASSERT_NE(value, nullptr);
    ASSERT_NE(max, nullptr);
    EXPECT_EQ(value->value, "101");
    EXPECT_EQ(max->value, "100");
    EXPECT_FALSE(error.Details().empty());
    EXPECT_FALSE(error.Hint().empty());
  } catch (...) {
    FAIL() << "Expected BlendOpacityOutOfRangeError";
  }
}

// ─── HSL modes with coloured pixels ──────────────────────────────────────────
//
// Source:      pure red   (255,   0,   0) — Lum≈77,  Sat=255
// Destination: pure blue  (  0,   0, 255) — Lum≈29,  Sat=255
//
// kHue:        hue(src=red) + sat(dst=255) + lum(dst≈29)
//              SetSat(red, 255) → (255,0,0); SetLum(→,29) → ClipColor clips to
//              (62,0,0)  ... Actually computed below via the same algorithm.
//
// We compute expected values analytically using the same integer arithmetic as
// the implementation so that the test is self-consistent.

namespace {

// Mirror of the implementation helpers (integer, 0-255 domain).
int LumI(int r, int g, int b) { return (77 * r + 150 * g + 29 * b) >> 8; }
int SatI(int r, int g, int b) {
  return std::max({r, g, b}) - std::min({r, g, b});
}

void ClipColor(int &r, int &g, int &b) {
  int lum = LumI(r, g, b);
  int mn  = std::min({r, g, b});
  int mx  = std::max({r, g, b});
  if (mn < 0) {
    r = lum + (r - lum) * lum / (lum - mn);
    g = lum + (g - lum) * lum / (lum - mn);
    b = lum + (b - lum) * lum / (lum - mn);
  }
  if (mx > 255) {
    r = lum + (r - lum) * (255 - lum) / (mx - lum);
    g = lum + (g - lum) * (255 - lum) / (mx - lum);
    b = lum + (b - lum) * (255 - lum) / (mx - lum);
  }
}

void SetLum(int &r, int &g, int &b, int lum) {
  int d = lum - LumI(r, g, b);
  r += d;
  g += d;
  b += d;
  ClipColor(r, g, b);
}

void SetSat(int &r, int &g, int &b, int sat) {
  int *p[3] = {&r, &g, &b};
  if (*p[0] > *p[1]) std::swap(p[0], p[1]);
  if (*p[1] > *p[2]) std::swap(p[1], p[2]);
  if (*p[0] > *p[1]) std::swap(p[0], p[1]);
  if (*p[2] > *p[0]) {
    *p[1] = (*p[1] - *p[0]) * sat / (*p[2] - *p[0]);
    *p[2] = sat;
  } else {
    *p[1] = *p[2] = 0;
  }
  *p[0] = 0;
}

struct RGB {
  std::uint8_t r, g, b;
};

RGB BlendHue(RGB d, RGB s) {
  int r = s.r, g = s.g, b = s.b;
  SetSat(r, g, b, SatI(d.r, d.g, d.b));
  SetLum(r, g, b, LumI(d.r, d.g, d.b));
  return {static_cast<std::uint8_t>(std::clamp(r, 0, 255)),
          static_cast<std::uint8_t>(std::clamp(g, 0, 255)),
          static_cast<std::uint8_t>(std::clamp(b, 0, 255))};
}

RGB BlendSaturation(RGB d, RGB s) {
  int r = d.r, g = d.g, b = d.b;
  SetSat(r, g, b, SatI(s.r, s.g, s.b));
  SetLum(r, g, b, LumI(d.r, d.g, d.b));
  return {static_cast<std::uint8_t>(std::clamp(r, 0, 255)),
          static_cast<std::uint8_t>(std::clamp(g, 0, 255)),
          static_cast<std::uint8_t>(std::clamp(b, 0, 255))};
}

RGB BlendColor(RGB d, RGB s) {
  int r = s.r, g = s.g, b = s.b;
  SetLum(r, g, b, LumI(d.r, d.g, d.b));
  return {static_cast<std::uint8_t>(std::clamp(r, 0, 255)),
          static_cast<std::uint8_t>(std::clamp(g, 0, 255)),
          static_cast<std::uint8_t>(std::clamp(b, 0, 255))};
}

RGB BlendLuminosity(RGB d, RGB s) {
  int r = d.r, g = d.g, b = d.b;
  SetLum(r, g, b, LumI(s.r, s.g, s.b));
  return {static_cast<std::uint8_t>(std::clamp(r, 0, 255)),
          static_cast<std::uint8_t>(std::clamp(g, 0, 255)),
          static_cast<std::uint8_t>(std::clamp(b, 0, 255))};
}

Buffer MakeColoredBuffer(RGB color) {
  // 1×1 RGBA opaque pixel
  return Buffer(1, 1, Color::kRgba, {color.r, color.g, color.b, 0xff});
}

}  // namespace

TEST(HslBlendingTest, HueColoredPixels) {
  const RGB src = {255, 0, 0};  // red
  const RGB dst = {0, 0, 255};  // blue

  auto source      = MakeColoredBuffer(src);
  auto destination = MakeColoredBuffer(dst);
  BlendInPlace(destination, source, Blending::kHue, Alpha::kStraight);

  const auto expected = BlendHue(dst, src);
  EXPECT_EQ(destination[0][0], expected.r) << "R";
  EXPECT_EQ(destination[0][1], expected.g) << "G";
  EXPECT_EQ(destination[0][2], expected.b) << "B";
}

TEST(HslBlendingTest, SaturationColoredPixels) {
  const RGB src = {255, 128, 0};    // orange — high saturation
  const RGB dst = {100, 100, 100};  // grey — zero saturation

  auto source      = MakeColoredBuffer(src);
  auto destination = MakeColoredBuffer(dst);
  BlendInPlace(destination, source, Blending::kSaturation, Alpha::kStraight);

  const auto expected = BlendSaturation(dst, src);
  EXPECT_EQ(destination[0][0], expected.r) << "R";
  EXPECT_EQ(destination[0][1], expected.g) << "G";
  EXPECT_EQ(destination[0][2], expected.b) << "B";
}

TEST(HslBlendingTest, SaturationGreySourceDesaturates) {
  // Grey source (Sat=0) blended onto coloured destination → result is grey
  const RGB src = {128, 128, 128};
  const RGB dst = {255, 0, 0};

  auto source      = MakeColoredBuffer(src);
  auto destination = MakeColoredBuffer(dst);
  BlendInPlace(destination, source, Blending::kSaturation, Alpha::kStraight);

  // Sat(src)=0 → result has Sat=0, i.e. R==G==B
  EXPECT_EQ(destination[0][0], destination[0][1]);
  EXPECT_EQ(destination[0][1], destination[0][2]);
}

TEST(HslBlendingTest, ColorColoredPixels) {
  const RGB src = {0, 255, 0};    // green
  const RGB dst = {128, 128, 0};  // olive

  auto source      = MakeColoredBuffer(src);
  auto destination = MakeColoredBuffer(dst);
  BlendInPlace(destination, source, Blending::kColor, Alpha::kStraight);

  const auto expected = BlendColor(dst, src);
  EXPECT_EQ(destination[0][0], expected.r) << "R";
  EXPECT_EQ(destination[0][1], expected.g) << "G";
  EXPECT_EQ(destination[0][2], expected.b) << "B";
}

TEST(HslBlendingTest, ColorPreservesLuminosity) {
  // kColor must preserve the destination luminosity
  const RGB src = {255, 0, 0};     // red — Lum≈30
  const RGB dst = {200, 200, 50};  // yellowish — Lum≈175

  auto source      = MakeColoredBuffer(src);
  auto destination = MakeColoredBuffer(dst);
  BlendInPlace(destination, source, Blending::kColor, Alpha::kStraight);

  const int result_lum =
      LumI(destination[0][0], destination[0][1], destination[0][2]);
  const int dst_lum = LumI(dst.r, dst.g, dst.b);
  EXPECT_EQ(result_lum, dst_lum);
}

TEST(HslBlendingTest, LuminosityColoredPixels) {
  const RGB src = {200, 200, 200};  // bright grey — high lum
  const RGB dst = {255, 0, 0};      // red

  auto source      = MakeColoredBuffer(src);
  auto destination = MakeColoredBuffer(dst);
  BlendInPlace(destination, source, Blending::kLuminosity, Alpha::kStraight);

  const auto expected = BlendLuminosity(dst, src);
  EXPECT_EQ(destination[0][0], expected.r) << "R";
  EXPECT_EQ(destination[0][1], expected.g) << "G";
  EXPECT_EQ(destination[0][2], expected.b) << "B";
}

TEST(HslBlendingTest, LuminosityPreservesHueSaturation) {
  // kLuminosity must preserve hue+sat of destination, i.e. Sat of result > 0
  const RGB src = {128, 128, 128};  // grey source
  const RGB dst = {255, 0, 0};      // fully saturated red

  auto source      = MakeColoredBuffer(src);
  auto destination = MakeColoredBuffer(dst);
  BlendInPlace(destination, source, Blending::kLuminosity, Alpha::kStraight);

  // Result must still be saturated (R != G or R != B)
  EXPECT_NE(destination[0][0], destination[0][1]);
}

// ─── kDissolve ───────────────────────────────────────────────────────────────

TEST(DissolveBlendingTest, FullyOpaqueSourceAlwaysShows) {
  // Alpha=255: every pixel must be replaced by the source colour
  Buffer source(3, 3, Color::kRgba,
                std::vector<std::uint8_t>(9 * 4, 0));  // black, alpha=255
  // set alpha to 255
  for (std::size_t i = 3; i < 9 * 4; i += 4) source.Data()[i] = 255;
  // set colour to red
  for (std::size_t i = 0; i < 9 * 4; i += 4) {
    source.Data()[i] = 200;
  }

  Buffer destination(3, 3, Color::kRgba, std::vector<std::uint8_t>(9 * 4, 128));

  BlendInPlace(destination, source, Blending::kDissolve, Alpha::kStraight);

  for (std::size_t i = 0; i < 9; ++i) {
    EXPECT_EQ(destination[i][0], 200) << "pixel " << i << " R";
    EXPECT_EQ(destination[i][3], 255) << "pixel " << i << " A";
  }
}

TEST(DissolveBlendingTest, FullyTransparentSourceNeverShows) {
  // Alpha=0: no pixel should change
  Buffer source(3, 3, Color::kRgba, std::vector<std::uint8_t>(9 * 4, 0));
  // alpha stays 0, colour = red
  for (std::size_t i = 0; i < 9 * 4; i += 4) source.Data()[i] = 200;

  const std::uint8_t fill = 77;
  Buffer destination(3, 3, Color::kRgba,
                     std::vector<std::uint8_t>(9 * 4, fill));

  BlendInPlace(destination, source, Blending::kDissolve, Alpha::kStraight);

  for (std::size_t i = 0; i < 9; ++i) {
    EXPECT_EQ(destination[i][0], fill) << "pixel " << i << " R";
    EXPECT_EQ(destination[i][1], fill) << "pixel " << i << " G";
  }
}

TEST(DissolveBlendingTest, Deterministic) {
  // Two identical calls must produce the same result
  auto make_src = [] {
    Buffer b(4, 4, Color::kRgba, std::vector<std::uint8_t>(4 * 4 * 4, 0));
    for (std::size_t i = 3; i < 4 * 4 * 4; i += 4) b.Data()[i] = 128;
    for (std::size_t i = 0; i < 4 * 4 * 4; i += 4) b.Data()[i] = 255;
    return b;
  };
  auto make_dst = [] {
    return Buffer(4, 4, Color::kRgba, std::vector<std::uint8_t>(4 * 4 * 4, 50));
  };

  auto dst1 = make_dst();
  BlendInPlace(dst1, make_src(), Blending::kDissolve, Alpha::kStraight);

  auto dst2 = make_dst();
  BlendInPlace(dst2, make_src(), Blending::kDissolve, Alpha::kStraight);

  for (std::size_t i = 0; i < 4 * 4; ++i) {
    EXPECT_EQ(dst1[i][0], dst2[i][0]) << "pixel " << i;
    EXPECT_EQ(dst1[i][3], dst2[i][3]) << "pixel " << i << " A";
  }
}

TEST(DissolveBlendingTest, PartialAlphaProducesHardPixels) {
  // With partial alpha, appeared pixels must have alpha=255 (hard edge)
  Buffer source(4, 4, Color::kRgba, std::vector<std::uint8_t>(4 * 4 * 4, 0));
  for (std::size_t i = 3; i < 4 * 4 * 4; i += 4) source.Data()[i] = 128;
  for (std::size_t i = 0; i < 4 * 4 * 4; i += 4) source.Data()[i] = 200;

  Buffer destination(4, 4, Color::kRgba,
                     std::vector<std::uint8_t>(4 * 4 * 4, 50));

  BlendInPlace(destination, source, Blending::kDissolve, Alpha::kStraight);

  for (std::size_t i = 0; i < 4 * 4; ++i) {
    const auto a = destination[i][3];
    // Each pixel is either the original (alpha=50) or the source (alpha=255)
    EXPECT_TRUE(a == 50 || a == 255) << "pixel " << i << " alpha=" << (int)a;
  }
}

TEST(DissolveBlendingTest, ZeroOpacityLeavesDestinationUnchanged) {
  Buffer source(3, 3, Color::kRgba, std::vector<std::uint8_t>(9 * 4, 0));
  for (std::size_t i = 3; i < 9 * 4; i += 4) source.Data()[i] = 255;
  for (std::size_t i = 0; i < 9 * 4; i += 4) source.Data()[i] = 200;

  auto destination    = MakeUniformBuffer(3, 3, 20, 30, 40, 50);
  const auto expected = destination.Clone();

  BlendInPlace(destination, source, Blending::kDissolve, Alpha::kStraight, 0);

  EXPECT_EQ(destination, expected);
}

TEST(DissolveBlendingTest, HalfOpacityIsDeterministicAndPartial) {
  Buffer source(4, 4, Color::kRgba, std::vector<std::uint8_t>(4 * 4 * 4, 0));
  for (std::size_t i = 3; i < 4 * 4 * 4; i += 4) source.Data()[i] = 255;
  for (std::size_t i = 0; i < 4 * 4 * 4; i += 4) source.Data()[i] = 220;

  auto make_dst = [] { return MakeUniformBuffer(4, 4, 25, 35, 45, 55); };

  auto dst1           = make_dst();
  const auto baseline = dst1.Clone();
  BlendInPlace(dst1, source, Blending::kDissolve, Alpha::kStraight, 50);

  auto dst2 = make_dst();
  BlendInPlace(dst2, source, Blending::kDissolve, Alpha::kStraight, 50);

  EXPECT_EQ(dst1, dst2);

  const auto changed = CountChangedPixels(dst1, baseline);
  EXPECT_GT(changed, 0u);
  EXPECT_LT(changed, dst1.PixelCount());
}

}  // namespace weqeqq::image
