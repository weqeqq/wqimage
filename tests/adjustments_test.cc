#include <gtest/gtest.h>
#include <weqeqq/image/adjustments.h>

#include <cstdint>
#include <vector>

namespace weqeqq::image {
namespace {

Buffer MakeRgba(std::vector<std::uint8_t> data) {
  return Buffer(3, 1, Color::kRgba, std::move(data));
}

Buffer MakeRgb(std::vector<std::uint8_t> data) {
  return Buffer(3, 1, Color::kRgb, std::move(data));
}

Buffer MakeGray(std::vector<std::uint8_t> data) {
  const auto width = data.size();
  return Buffer(width, 1, Color::kGrayscale, std::move(data));
}

TEST(AdjustmentsTest, AdjustBrightnessReturnsCloneForZeroValue) {
  const auto input  = MakeRgb({10, 20, 30, 40, 50, 60, 70, 80, 90});
  const auto output = AdjustBrightness(input, 0);

  EXPECT_EQ(output, input);
}

TEST(AdjustmentsTest, ModernBrightnessAdjustsRgbChannelsAndPreservesAlpha) {
  auto buffer = MakeRgba({
      0,
      32,
      64,
      7,
      96,
      128,
      160,
      9,
      192,
      224,
      255,
      11,
  });

  AdjustBrightnessInPlace(buffer, 60, AdjustmentMode::kModern);

  const auto expected = MakeRgba({
      102,
      121,
      140,
      7,
      160,
      179,
      198,
      9,
      217,
      236,
      255,
      11,
  });
  EXPECT_EQ(buffer, expected);
}

TEST(AdjustmentsTest, LegacyBrightnessUsesDirectShift) {
  auto buffer = MakeRgb({
      0,
      32,
      64,
      96,
      128,
      160,
      192,
      224,
      255,
  });

  AdjustBrightnessInPlace(buffer, -60, AdjustmentMode::kLegacy);

  const auto expected = MakeRgb({
      0,
      0,
      4,
      36,
      68,
      100,
      132,
      164,
      195,
  });
  EXPECT_EQ(buffer, expected);
}

TEST(AdjustmentsTest, ModernContrastSupportsGrayscale) {
  auto buffer = MakeGray({0, 64, 128, 192, 255});

  AdjustContrastInPlace(buffer, 25, AdjustmentMode::kModern);

  const auto expected = MakeGray({0, 43, 128, 213, 255});
  EXPECT_EQ(buffer, expected);
}

TEST(AdjustmentsTest, LegacyContrastUsesLegacyCurve) {
  auto buffer = MakeGray({0, 32, 64, 96, 128, 160, 192, 224, 255});

  AdjustContrastInPlace(buffer, 50, AdjustmentMode::kLegacy);

  const auto expected = MakeGray({0, 0, 0, 33, 128, 223, 255, 255, 255});
  EXPECT_EQ(buffer, expected);
}

TEST(AdjustmentsTest, ParallelExecutionMatchesSequential) {
  std::vector<std::uint8_t> data(4096 * 4);
  for (std::size_t i = 0; i < data.size(); ++i) {
    data[i] = static_cast<std::uint8_t>(i % 251);
  }

  auto sequential = Buffer(4096, 1, Color::kRgba, data);
  auto parallel   = Buffer(4096, 1, Color::kRgba, std::move(data));

  AdjustContrastInPlace(sequential, 25, AdjustmentMode::kModern,
                        parallel::Execution::kSequential);
  AdjustContrastInPlace(parallel, 25, AdjustmentMode::kModern,
                        parallel::Execution::kParallel);

  EXPECT_EQ(parallel, sequential);
}

TEST(AdjustmentsTest, UnsupportedColorThrowsTypedError) {
  Buffer buffer(1, 1, Color::kCmyk, {0, 0, 0, 0});

  EXPECT_THROW((void)AdjustBrightness(buffer, 10),
               AdjustmentUnsupportedColorError);
}

TEST(AdjustmentsTest, BrightnessOutOfRangeErrorExposesStructuredMetadata) {
  const auto input = MakeGray({10, 20, 30});

  try {
    (void)AdjustBrightness(input, 151);
    FAIL() << "Expected AdjustmentBrightnessOutOfRangeError";
  } catch (const AdjustmentBrightnessOutOfRangeError& error) {
    EXPECT_EQ(error.TypedCode(), ErrorCode::kAdjustmentBrightnessOutOfRange);

    const auto* value = error.FindField("value");
    const auto* max   = error.FindField("max");
    ASSERT_NE(value, nullptr);
    ASSERT_NE(max, nullptr);
    EXPECT_EQ(value->value, "151");
    EXPECT_EQ(max->value, "150");
    EXPECT_FALSE(error.Details().empty());
    EXPECT_FALSE(error.Hint().empty());
  } catch (...) {
    FAIL() << "Expected AdjustmentBrightnessOutOfRangeError";
  }
}

TEST(AdjustmentsTest, ContrastOutOfRangeErrorExposesStructuredMetadata) {
  const auto input = MakeGray({10, 20, 30});

  try {
    (void)AdjustContrast(input, -51);
    FAIL() << "Expected AdjustmentContrastOutOfRangeError";
  } catch (const AdjustmentContrastOutOfRangeError& error) {
    EXPECT_EQ(error.TypedCode(), ErrorCode::kAdjustmentContrastOutOfRange);

    const auto* value = error.FindField("value");
    const auto* min   = error.FindField("min");
    ASSERT_NE(value, nullptr);
    ASSERT_NE(min, nullptr);
    EXPECT_EQ(value->value, "-51");
    EXPECT_EQ(min->value, "-50");
    EXPECT_FALSE(error.Details().empty());
    EXPECT_FALSE(error.Hint().empty());
  } catch (...) {
    FAIL() << "Expected AdjustmentContrastOutOfRangeError";
  }
}

TEST(AdjustmentsTest, InvalidModeThrowsTypedError) {
  const auto input = MakeGray({10, 20, 30});

  EXPECT_THROW(
      (void)AdjustBrightness(input, 10, static_cast<AdjustmentMode>(255)),
      AdjustmentInvalidModeError);
}

}  // namespace
}  // namespace weqeqq::image
