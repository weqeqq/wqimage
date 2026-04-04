
#include <weqeqq/color.h>
#include <weqeqq/image/color_conversion.h>

namespace weqeqq::image {

Buffer ConvertColor(const Buffer &input, Color color, ColorStandard standard) {
  Buffer output(input.Width(), input.Height(), color);
  ConvertColor(input, output, standard);

  return output;
}

void ConvertColor(const Buffer &source, Buffer &destination,
                  ColorStandard standard) {
  color::Convert(source, destination, source.Color(), destination.Color(),
                 standard);
}

}  // namespace weqeqq::image
