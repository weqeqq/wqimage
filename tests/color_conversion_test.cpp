#include <cstddef>
#include <cstdint>
#include <vector>

import weqeqq.image;
import weqeqq.test;

namespace {
using namespace weqeqq::test;
namespace wi = weqeqq::image;

const Suite kColorConversion("color-conversion", [] {
  Test("rgb to rgba sets opaque alpha", [] {
    std::vector<std::uint8_t> data = {10, 20, 30, 40, 50, 60};
    wi::Buffer rgb(2, 1, wi::Color::kRgb, std::move(data));
    auto rgba = wi::ConvertColor(rgb, wi::Color::kRgba);
    Expect(rgba.ChannelCount(), Eq(std::size_t{4}));
    Expect(int(rgba[0, 0][0]), Eq(10));
    Expect(int(rgba[0, 0][3]), Eq(255));
    Expect(int(rgba[1, 0][2]), Eq(60));
  });

  Test("rgb to grayscale preserves dimensions", [] {
    std::vector<std::uint8_t> data(4 * 2 * 3, 128);
    wi::Buffer rgb(4, 2, wi::Color::kRgb, std::move(data));
    auto gray = wi::ConvertColor(rgb, wi::Color::kGrayscale);
    Expect(gray.Width(), Eq(std::size_t{4}));
    Expect(gray.Height(), Eq(std::size_t{2}));
    Expect(gray.ChannelCount(), Eq(std::size_t{1}));
  });
});

}  // namespace
