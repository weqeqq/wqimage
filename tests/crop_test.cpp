#include <cstddef>
#include <cstdint>
#include <vector>

import weqeqq.image;
import weqeqq.image.processing;
import weqeqq.test;

namespace {
using namespace weqeqq::test;
namespace wi = weqeqq::image;

wi::Buffer MakeGradient(std::size_t width, std::size_t height) {
  std::vector<std::uint8_t> data(width * height);
  for (std::size_t y = 0; y < height; ++y) {
    for (std::size_t x = 0; x < width; ++x) {
      data[y * width + x] = static_cast<std::uint8_t>(y * width + x);
    }
  }
  return wi::Buffer(width, height, wi::Color::kGrayscale, std::move(data));
}

const Suite kCrop("crop", [] {
  Test("crop copies the requested region", [] {
    auto image   = MakeGradient(4, 4);
    auto cropped = wi::Crop(image, 1, 1, 2, 2);
    Expect(cropped.Width(), Eq(std::size_t{2}));
    Expect(cropped.Height(), Eq(std::size_t{2}));
    Expect(int(cropped[0, 0][0]), Eq(5));
    Expect(int(cropped[1, 0][0]), Eq(6));
    Expect(int(cropped[0, 1][0]), Eq(9));
    Expect(int(cropped[1, 1][0]), Eq(10));
  });

  Test("crop rejects zero dimensions", [] {
    auto image = MakeGradient(4, 4);
    Expect([&] { wi::Crop(image, 0, 0, 0, 2); },
           Throws<weqeqq::error::Error>());
  });

  Test("crop rejects origin out of bounds", [] {
    auto image = MakeGradient(4, 4);
    Expect([&] { wi::Crop(image, 4, 0, 1, 1); },
           Throws<weqeqq::error::Error>());
  });

  Test("crop rejects extent out of bounds", [] {
    auto image = MakeGradient(4, 4);
    Expect([&] { wi::Crop(image, 2, 2, 3, 3); },
           Throws<weqeqq::error::Error>());
  });
});

}  // namespace
