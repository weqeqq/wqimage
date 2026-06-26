module;

#include <cstdint>
#include <span>
#include <utility>

module weqeqq.image.io;

import weqeqq.png;
import weqeqq.avif;

// Implementation unit for weqeqq.image.io:decoding. It owns the BMI dependency
// on weqeqq.png and weqeqq.avif; because these imports live in a module
// implementation unit they do not leak to consumers of weqeqq.image.io. The
// declarations of the functions defined below and the throw helpers they use
// come from the implicitly imported primary module interface.
namespace weqeqq::image::internal {

bool HasPngSignature(std::span<const std::uint8_t> data) {
  return png::HasPngSignature(data);
}

bool HasAvifSignature(std::span<const std::uint8_t> data) {
  return avif::HasAvifSignature(data);
}

Buffer DecodePng(std::span<const std::uint8_t> data, Color color) {
  try {
    auto decoded = png::DecodeImage(data, color);
    return Buffer(decoded.info.width, decoded.info.height, color,
                  std::move(decoded.data));
  } catch (const error::Error& cause) {
    ThrowDecodeFailed(data.size(), color, cause.what());
  }
}

Buffer DecodeAvif(std::span<const std::uint8_t> data, Color color) {
  try {
    auto decoded = avif::DecodeImage(data, color);
    return Buffer(decoded.info.width, decoded.info.height, color,
                  std::move(decoded.data));
  } catch (const error::Error& cause) {
    ThrowDecodeFailed(data.size(), color, cause.what());
  }
}

}  // namespace weqeqq::image::internal
