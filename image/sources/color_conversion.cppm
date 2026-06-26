module;

#include <span>
#include <utility>

export module weqeqq.image:color_conversion;

import :buffer;
export import weqeqq.color;
export import weqeqq.parallel;

export namespace weqeqq::image {

using ColorStandard = color::Standard;

inline constexpr auto ColorStandardCount =
    static_cast<int>(ColorStandard::kBt709) + 1;

void ConvertColor(const Buffer& source, Buffer& destination,
                  ColorStandard standard = ColorStandard::kBt709) {
  color::Convert(source, destination, source.Color(), destination.Color(),
                 standard);
}

[[nodiscard]] Buffer ConvertColor(
    const Buffer& input, Color color,
    ColorStandard standard = ColorStandard::kBt709) {
  Buffer output(input.Width(), input.Height(), color);
  ConvertColor(input, output, standard);
  return output;
}

}  // namespace weqeqq::image
