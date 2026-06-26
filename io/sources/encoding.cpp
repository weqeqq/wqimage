module;

#include <cstdint>
#include <vector>

module weqeqq.image.io;

import weqeqq.png;
import weqeqq.avif;

// Implementation unit for weqeqq.image.io:encoding. It owns the BMI dependency
// on weqeqq.png and weqeqq.avif; because these imports live in a module
// implementation unit they do not leak to consumers of weqeqq.image.io. The
// declarations of the functions defined below and the throw helpers they use
// come from the implicitly imported primary module interface.
namespace weqeqq::image::internal {

std::vector<std::uint8_t> EncodePng(const Buffer& buffer) {
  try {
    return png::EncodeImage(buffer, buffer.Width(), buffer.Height(),
                            buffer.Color());
  } catch (const error::Error& cause) {
    ThrowEncodeFailed(buffer, cause.what());
  }
}

std::vector<std::uint8_t> EncodeAvif(const Buffer& buffer) {
  try {
    return avif::EncodeImage(buffer, buffer.Width(), buffer.Height(),
                             buffer.Color());
  } catch (const error::Error& cause) {
    ThrowEncodeFailed(buffer, cause.what());
  }
}

}  // namespace weqeqq::image::internal
