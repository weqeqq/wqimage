#include <cstddef>
#include <cstdint>
#include <vector>

import weqeqq.image;
import weqeqq.test;

namespace {
using namespace weqeqq::test;
namespace wi = weqeqq::image;

const Suite kBuffer("buffer", [] {
  Test("zeroed buffer reports metadata and sizes", [] {
    wi::Buffer buffer(4, 3, wi::Color::kRgba);
    Expect(buffer.Width(), Eq(std::size_t{4}));
    Expect(buffer.Height(), Eq(std::size_t{3}));
    Expect(buffer.ChannelCount(), Eq(std::size_t{4}));
    Expect(buffer.PixelCount(), Eq(std::size_t{12}));
    Expect(buffer.StrideBytes(), Eq(std::size_t{16}));
    Expect(buffer.ByteCount(), Eq(std::size_t{48}));
    Expect(buffer.Empty(), IsFalse());
    Expect(int(buffer[0, 0][0]), Eq(0));
  });

  Test("buffer from data accepts exact size", [] {
    std::vector<std::uint8_t> data(2 * 2 * 3, 7);
    wi::Buffer buffer(2, 2, wi::Color::kRgb, std::move(data));
    Expect(buffer.ByteCount(), Eq(std::size_t{12}));
    Expect(int(buffer[1, 1][2]), Eq(7));
  });

  Test("buffer rejects zero dimensions", [] {
    Expect([] { wi::Buffer buffer(0, 4, wi::Color::kRgb); },
           Throws<weqeqq::error::Error>());
  });

  Test("buffer rejects data size mismatch", [] {
    Expect(
        [] {
          std::vector<std::uint8_t> data(5);
          wi::Buffer buffer(2, 2, wi::Color::kRgb, std::move(data));
        },
        Throws<weqeqq::error::Error>());
  });

  Test("2d and linear indexing agree", [] {
    wi::Buffer buffer(3, 2, wi::Color::kGrayscale);
    buffer[2, 1][0] = 42;
    Expect(int(buffer[5][0]), Eq(42));
  });

  Test("clone is equal and independent", [] {
    std::vector<std::uint8_t> data(2 * 1 * 4, 9);
    wi::Buffer buffer(2, 1, wi::Color::kRgba, std::move(data));
    auto clone = buffer.Clone();
    Expect(clone == buffer, IsTrue());
    clone[0, 0][0] = 1;
    Expect(int(buffer[0, 0][0]), Eq(9));
  });
});

const Suite kBufferInfo("buffer-info", [] {
  Test("channel and byte counts", [] {
    wi::BufferInfo info{.width = 10, .height = 5, .color = wi::Color::kRgba};
    Expect(info.ChannelCount(), Eq(std::size_t{4}));
    Expect(info.PixelCount(), Eq(std::size_t{50}));
    Expect(info.StrideBytes(), Eq(std::size_t{40}));
    Expect(info.ByteCount(), Eq(std::size_t{200}));
  });
});

}  // namespace
