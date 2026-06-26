#include <cstddef>
#include <cstdint>
#include <vector>

import weqeqq.image;
import weqeqq.image.processing;
import weqeqq.test;

namespace {
using namespace weqeqq::test;
namespace wi = weqeqq::image;

wi::Buffer MakeRgb(std::uint8_t value) {
  std::vector<std::uint8_t> data(2 * 2 * 3, value);
  return wi::Buffer(2, 2, wi::Color::kRgb, std::move(data));
}

const Suite kAdjustments("adjustments", [] {
  Test("brightness zero is identity", [] {
    auto image      = MakeRgb(100);
    auto brightened = wi::AdjustBrightness(image, 0, wi::AdjustmentMode::kModern);
    Expect(int(brightened[0, 0][0]), Eq(100));
  });

  Test("legacy brightness adds and clamps", [] {
    auto image  = MakeRgb(200);
    auto result = wi::AdjustBrightness(image, 100, wi::AdjustmentMode::kLegacy);
    Expect(int(result[0, 0][0]), Eq(255));
  });

  Test("contrast leaves rgba alpha untouched", [] {
    std::vector<std::uint8_t> data = {200, 200, 200, 17};
    wi::Buffer image(1, 1, wi::Color::kRgba, std::move(data));
    wi::AdjustContrastInPlace(image, 50, wi::AdjustmentMode::kModern);
    Expect(int(image[0, 0][3]), Eq(17));
  });

  Test("brightness rejects out of range", [] {
    auto image = MakeRgb(100);
    Expect([&] { wi::AdjustBrightness(image, 1000); },
           Throws<weqeqq::error::Error>());
  });

  Test("adjustments reject unsupported color", [] {
    std::vector<std::uint8_t> data(1 * 1 * 4, 0);
    wi::Buffer cmyk(1, 1, wi::Color::kCmyk, std::move(data));
    Expect([&] { wi::AdjustBrightnessInPlace(cmyk, 10); },
           Throws<weqeqq::error::Error>());
  });
});

}  // namespace
