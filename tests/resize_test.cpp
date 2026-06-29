#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

import weqeqq.image;
import weqeqq.image.processing;
import weqeqq.test;

namespace {
using namespace weqeqq::test;
namespace wi = weqeqq::image;

std::size_t ChannelsOf(wi::Color color) {
  switch (color) {
    case wi::Color::kGrayscale:
      return 1;
    case wi::Color::kRgb:
      return 3;
    case wi::Color::kRgba:
    case wi::Color::kCmyk:
      return 4;
    default:
      return 0;
  }
}

wi::Buffer MakeSolid(std::size_t width, std::size_t height, wi::Color color,
                     std::uint8_t value) {
  std::vector<std::uint8_t> data(width * height * ChannelsOf(color), value);
  return wi::Buffer(width, height, color, std::move(data));
}

const Suite kResize("resize", [] {
  Test("resize produces requested dimensions and color", [] {
    auto image  = MakeSolid(4, 4, wi::Color::kRgb, 100);
    auto result = wi::Resize(image, 8, 6, wi::Filter::Lanczos3);
    Expect(result.Width(), Eq(std::size_t{8}));
    Expect(result.Height(), Eq(std::size_t{6}));
    Expect(result.Color() == wi::Color::kRgb, IsTrue());
  });

  Test("nearest 2x upscale replicates source pixels", [] {
    std::vector<std::uint8_t> data = {10, 20, 30, 40};
    wi::Buffer image(2, 2, wi::Color::kGrayscale, std::move(data));
    auto result =
        wi::Resize(image, 4, 4, wi::Filter::Nearest, wi::Gamma::kLinear);
    Expect(int(result[0, 0][0]), Eq(10));
    Expect(int(result[3, 0][0]), Eq(20));
    Expect(int(result[0, 3][0]), Eq(30));
    Expect(int(result[3, 3][0]), Eq(40));
  });

  Test("resize to same size is identity", [] {
    auto image  = MakeSolid(4, 4, wi::Color::kRgb, 123);
    auto result = wi::Resize(image, 4, 4, wi::Filter::Lanczos3);
    Expect(int(result[0, 0][0]), Eq(123));
    Expect(int(result[3, 3][2]), Eq(123));
  });

  Test("downscaling a constant image preserves the value", [] {
    const std::array filters = {
        wi::Filter::Nearest,  wi::Filter::Triangle, wi::Filter::CatmullRom,
        wi::Filter::Mitchell, wi::Filter::Lanczos3, wi::Filter::Box,
        wi::Filter::Lanczos2, wi::Filter::Hamming};
    for (auto filter : filters) {
      auto image  = MakeSolid(4, 4, wi::Color::kRgb, 200);
      auto result = wi::Resize(image, 2, 2, filter, wi::Gamma::kLinear);
      Expect(int(result[0, 0][0]), Near(200, 1));
      Expect(int(result[1, 1][2]), Near(200, 1));
    }
  });

  Test("box downscale averages neighbors", [] {
    std::vector<std::uint8_t> data = {0, 255};
    wi::Buffer image(2, 1, wi::Color::kGrayscale, std::move(data));
    auto result = wi::Resize(image, 1, 1, wi::Filter::Box, wi::Gamma::kLinear);
    Expect(int(result[0, 0][0]), Near(128, 1));
  });

  Test("triangle downscale averages neighbors", [] {
    std::vector<std::uint8_t> data = {0, 255};
    wi::Buffer image(2, 1, wi::Color::kGrayscale, std::move(data));
    auto result =
        wi::Resize(image, 1, 1, wi::Filter::Triangle, wi::Gamma::kLinear);
    Expect(int(result[0, 0][0]), Near(128, 1));
  });

  Test("rgba resize avoids transparent color bleed", [] {
    std::vector<std::uint8_t> data = {255, 0, 0, 0, 0, 0, 255, 255};
    wi::Buffer image(2, 1, wi::Color::kRgba, std::move(data));
    auto result = wi::Resize(image, 1, 1, wi::Filter::Box, wi::Gamma::kLinear);
    Expect(int(result[0, 0][0]), Near(0, 1));
    Expect(int(result[0, 0][2]), Near(255, 1));
    Expect(int(result[0, 0][3]), Near(128, 1));
  });

  Test("gamma srgb differs from linear on a contrast edge", [] {
    std::vector<std::uint8_t> data = {0, 255};  // 2x1 grayscale
    wi::Buffer linear_src(2, 1, wi::Color::kGrayscale,
                          std::vector<std::uint8_t>{0, 255});
    wi::Buffer srgb_src(2, 1, wi::Color::kGrayscale, std::move(data));
    auto linear =
        wi::Resize(linear_src, 1, 1, wi::Filter::Box, wi::Gamma::kLinear);
    auto srgb = wi::Resize(srgb_src, 1, 1, wi::Filter::Box, wi::Gamma::kSrgb);
    Expect(int(srgb[0, 0][0]), Gt(int(linear[0, 0][0])));
  });

  Test("all filters handle all color formats", [] {
    const std::array filters = {
        wi::Filter::Nearest,  wi::Filter::Triangle, wi::Filter::CatmullRom,
        wi::Filter::Mitchell, wi::Filter::Lanczos3, wi::Filter::Box,
        wi::Filter::Lanczos2, wi::Filter::Hamming};
    const std::array colors = {wi::Color::kRgb, wi::Color::kRgba,
                               wi::Color::kGrayscale, wi::Color::kCmyk};
    for (auto color : colors) {
      for (auto filter : filters) {
        auto image = MakeSolid(4, 4, color, 90);
        Expect([&] { auto r = wi::Resize(image, 3, 5, filter); (void)r; },
               NoThrows());
      }
    }
  });

  Test("ResizeByScale maps factors to dimensions", [] {
    auto image  = MakeSolid(4, 4, wi::Color::kRgb, 50);
    auto result = wi::ResizeByScale(image, 2.0);
    Expect(result.Width(), Eq(std::size_t{8}));
    Expect(result.Height(), Eq(std::size_t{8}));
  });

  Test("resize rejects zero dimensions", [] {
    auto image = MakeSolid(4, 4, wi::Color::kRgb, 10);
    Expect([&] { wi::Resize(image, 0, 4); },
           Throws<weqeqq::error::Error>());
  });

  Test("resize rejects empty input", [] {
    wi::Buffer empty;
    Expect([&] { wi::Resize(empty, 4, 4); },
           Throws<weqeqq::error::Error>());
  });

  Test("ResizeByScale rejects non-positive scale", [] {
    auto image = MakeSolid(4, 4, wi::Color::kRgb, 10);
    Expect([&] { wi::ResizeByScale(image, 0.0); },
           Throws<weqeqq::error::Error>());
    Expect([&] { wi::ResizeByScale(image, -1.0); },
           Throws<weqeqq::error::Error>());
  });
});

}  // namespace
