#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

import weqeqq.image;
import weqeqq.image.io;
import weqeqq.test;

namespace {
using namespace weqeqq::test;
namespace wi = weqeqq::image;

wi::Buffer MakePattern(std::size_t width, std::size_t height) {
  std::vector<std::uint8_t> data(width * height * 3);
  for (std::size_t i = 0; i < data.size(); ++i) {
    data[i] = static_cast<std::uint8_t>((i * 37) & 0xff);
  }
  return wi::Buffer(width, height, wi::Color::kRgb, std::move(data));
}

const Suite kIo("io", [] {
  Test("png encode then decode round trips losslessly", [] {
    auto image    = MakePattern(16, 12);
    auto encoded  = wi::Encode(image, wi::Format::kPng);
    auto decoded  = wi::Decode(std::span<const std::uint8_t>(encoded),
                               wi::Color::kRgb);
    Expect(decoded.Width(), Eq(std::size_t{16}));
    Expect(decoded.Height(), Eq(std::size_t{12}));
    Expect(decoded.Color() == wi::Color::kRgb, IsTrue());
    Expect(decoded == image, IsTrue());
  });

  Test("avif encode then decode preserves geometry", [] {
    auto image   = MakePattern(16, 12);
    auto encoded = wi::Encode(image, wi::Format::kAvif);
    auto decoded = wi::Decode(std::span<const std::uint8_t>(encoded),
                              wi::Color::kRgb);
    Expect(decoded.Width(), Eq(std::size_t{16}));
    Expect(decoded.Height(), Eq(std::size_t{12}));
    Expect(decoded.Color() == wi::Color::kRgb, IsTrue());
  });

  Test("decode rejects unsupported input", [] {
    std::vector<std::uint8_t> garbage = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    Expect(
        [&] {
          wi::Decode(std::span<const std::uint8_t>(garbage), wi::Color::kRgb);
        },
        Throws<weqeqq::error::Error>());
  });
});

}  // namespace
