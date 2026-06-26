#include <cstddef>
#include <cstdint>
#include <vector>

import weqeqq.image;
import weqeqq.image.processing;
import weqeqq.test;

namespace {
using namespace weqeqq::test;
namespace wi = weqeqq::image;

wi::Buffer MakeRgba(std::size_t width, std::size_t height, std::uint8_t r,
                    std::uint8_t g, std::uint8_t b, std::uint8_t a) {
  std::vector<std::uint8_t> data(width * height * 4);
  for (std::size_t i = 0; i < width * height; ++i) {
    data[i * 4 + 0] = r;
    data[i * 4 + 1] = g;
    data[i * 4 + 2] = b;
    data[i * 4 + 3] = a;
  }
  return wi::Buffer(width, height, wi::Color::kRgba, std::move(data));
}

wi::Buffer MakeRgb(std::size_t width, std::size_t height, std::uint8_t r,
                   std::uint8_t g, std::uint8_t b) {
  std::vector<std::uint8_t> data(width * height * 3);
  for (std::size_t i = 0; i < width * height; ++i) {
    data[i * 3 + 0] = r;
    data[i * 3 + 1] = g;
    data[i * 3 + 2] = b;
  }
  return wi::Buffer(width, height, wi::Color::kRgb, std::move(data));
}

const Suite kBlending("blending", [] {
  Test("normal opaque source replaces destination", [] {
    auto dst = MakeRgba(8, 8, 10, 20, 30, 255);
    auto src = MakeRgba(8, 8, 200, 150, 100, 255);
    wi::BlendInPlace(dst, src, wi::Blending::kNormal, wi::Alpha::kStraight);
    Expect(int(dst[3, 3][0]), Eq(200));
    Expect(int(dst[3, 3][1]), Eq(150));
    Expect(int(dst[3, 3][2]), Eq(100));
    Expect(int(dst[3, 3][3]), Eq(255));
  });

  Test("multiply blends rgb channels", [] {
    auto dst = MakeRgb(8, 8, 200, 200, 200);
    auto src = MakeRgb(8, 8, 128, 0, 255);
    wi::BlendInPlace(dst, src, wi::Blending::kMultiply);
    Expect(int(dst[1, 1][0]), Eq(100));
    Expect(int(dst[1, 1][1]), Eq(0));
    Expect(int(dst[1, 1][2]), Eq(200));
  });

  Test("zero opacity leaves destination unchanged", [] {
    auto dst = MakeRgb(8, 8, 10, 20, 30);
    auto src = MakeRgb(8, 8, 200, 200, 200);
    wi::BlendInPlace(dst, src, wi::Blending::kNormal, wi::Alpha::kStraight, 0);
    Expect(int(dst[0, 0][0]), Eq(10));
  });

  Test("blend rejects opacity out of range", [] {
    auto dst = MakeRgb(2, 2, 0, 0, 0);
    auto src = MakeRgb(2, 2, 1, 1, 1);
    Expect(
        [&] {
          wi::BlendInPlace(dst, src, wi::Blending::kNormal, wi::Alpha::kStraight,
                           200);
        },
        Throws<weqeqq::error::Error>());
  });

  Test("premultiply then unpremultiply round trips opaque pixels", [] {
    auto image = MakeRgba(9, 9, 200, 100, 50, 255);
    wi::PremultiplyInPlace(image);
    Expect(int(image[0, 0][0]), Eq(200));
    wi::UnpremultiplyInPlace(image);
    Expect(int(image[0, 0][0]), Eq(200));
    Expect(int(image[0, 0][1]), Eq(100));
    Expect(int(image[0, 0][2]), Eq(50));
  });

  Test("premultiply scales by alpha", [] {
    auto image = MakeRgba(9, 9, 200, 200, 200, 128);
    wi::PremultiplyInPlace(image);
    Expect(int(image[4, 4][0]), Eq(100));
    Expect(int(image[4, 4][3]), Eq(128));
  });
});

}  // namespace
