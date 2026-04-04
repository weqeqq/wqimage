
#pragma once

#include <weqeqq/color.h>
#include <weqeqq/image/buffer.h>
#include <weqeqq/image/export.h>

namespace weqeqq::image {

using ColorStandard = color::Standard;

inline constexpr auto ColorStandardCount =
    static_cast<int>(ColorStandard::kBt709) + 1;

[[nodiscard]]
WQIMAGE_EXPORT Buffer ConvertColor(
    const Buffer &input, Color color,
    ColorStandard standard = ColorStandard::kBt709);

WQIMAGE_EXPORT void ConvertColor(
    const Buffer &input, Buffer &output,
    ColorStandard standard = ColorStandard::kBt709);

}  // namespace weqeqq::image
