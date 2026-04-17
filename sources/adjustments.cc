#include <weqeqq/image/adjustments.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace weqeqq::image {
namespace {

using Lut = std::array<std::uint8_t, 256>;

constexpr auto kBrightnessRangeSize =
    static_cast<std::size_t>(kBrightnessMax - kBrightnessMin + 1);
constexpr auto kContrastRangeSize =
    static_cast<std::size_t>(kContrastMax - kContrastMin + 1);

[[nodiscard]]
std::uint8_t ClampToByte(double value) {
  return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0, 255.0)));
}

void ValidateMode(AdjustmentMode mode) {
  switch (mode) {
    case AdjustmentMode::kModern:
    case AdjustmentMode::kLegacy:
      return;
    case AdjustmentMode::kCount:
    default:
      throw AdjustmentInvalidModeError(mode);
  }
}

void ValidateAdjustmentColor(Color color) {
  switch (color) {
    case Color::kRgb:
    case Color::kRgba:
    case Color::kGrayscale:
      return;
    case Color::kCmyk:
    case Color::kCount:
    default:
      throw AdjustmentUnsupportedColorError(color);
  }
}

void ValidateBrightnessValue(int value, AdjustmentMode mode) {
  if (value < kBrightnessMin || value > kBrightnessMax) {
    throw AdjustmentBrightnessOutOfRangeError(value, kBrightnessMin,
                                              kBrightnessMax, mode);
  }
}

void ValidateContrastValue(int value, AdjustmentMode mode) {
  if (value < kContrastMin || value > kContrastMax) {
    throw AdjustmentContrastOutOfRangeError(value, kContrastMin, kContrastMax,
                                            mode);
  }
}

[[nodiscard]]
std::size_t AdjustedChannelCount(Color color) {
  switch (color) {
    case Color::kGrayscale:
      return 1;
    case Color::kRgb:
    case Color::kRgba:
      return 3;
    case Color::kCmyk:
    case Color::kCount:
    default:
      return 0;
  }
}

[[nodiscard]]
std::uint8_t ApplyBrightnessModern(int input, int value) {
  const auto brightness = static_cast<double>(value) / kBrightnessMax;
  const auto normalized = static_cast<double>(input) / 255.0;

  const auto output = brightness >= 0.0
                          ? normalized + (1.0 - normalized) * brightness
                          : normalized * (1.0 + brightness);

  return ClampToByte(output * 255.0);
}

[[nodiscard]]
std::uint8_t ApplyBrightnessLegacy(int input, int value) {
  return ClampToByte(static_cast<double>(input + value));
}

[[nodiscard]]
std::uint8_t ApplyContrastModern(int input, int value) {
  if (value == kContrastMax) {
    if (input < 128) {
      return 0;
    }
    if (input > 128) {
      return 255;
    }
    return 128;
  }

  const auto contrast = static_cast<double>(value) / 100.0;
  const auto factor = contrast < 0.0 ? 1.0 + contrast : 1.0 / (1.0 - contrast);

  return ClampToByte((static_cast<double>(input) - 128.0) * factor + 128.0);
}

[[nodiscard]]
std::uint8_t ApplyContrastLegacy(int input, int value) {
  const auto mapped = static_cast<double>(value) / 100.0 * 255.0;
  const auto factor = (259.0 * (mapped + 255.0)) / (255.0 * (259.0 - mapped));

  return ClampToByte((static_cast<double>(input) - 128.0) * factor + 128.0);
}

template <std::size_t Count, typename Function>
[[nodiscard]]
auto MakeLuts(int min_value, Function&& function) {
  std::array<Lut, Count> luts{};

  for (int value = min_value; value < min_value + static_cast<int>(Count);
       ++value) {
    auto& lut = luts[static_cast<std::size_t>(value - min_value)];
    for (int input = 0; input < static_cast<int>(lut.size()); ++input) {
      lut[static_cast<std::size_t>(input)] = function(input, value);
    }
  }

  return luts;
}

[[nodiscard]]
const auto& ModernBrightnessLuts() {
  static const auto luts = MakeLuts<kBrightnessRangeSize>(
      kBrightnessMin,
      [](int input, int value) { return ApplyBrightnessModern(input, value); });
  return luts;
}

[[nodiscard]]
const auto& LegacyBrightnessLuts() {
  static const auto luts = MakeLuts<kBrightnessRangeSize>(
      kBrightnessMin,
      [](int input, int value) { return ApplyBrightnessLegacy(input, value); });
  return luts;
}

[[nodiscard]]
const auto& ModernContrastLuts() {
  static const auto luts = MakeLuts<kContrastRangeSize>(
      kContrastMin,
      [](int input, int value) { return ApplyContrastModern(input, value); });
  return luts;
}

[[nodiscard]]
const auto& LegacyContrastLuts() {
  static const auto luts = MakeLuts<kContrastRangeSize>(
      kContrastMin,
      [](int input, int value) { return ApplyContrastLegacy(input, value); });
  return luts;
}

[[nodiscard]]
const Lut& SelectBrightnessLut(int value, AdjustmentMode mode) {
  const auto index = static_cast<std::size_t>(value - kBrightnessMin);

  switch (mode) {
    case AdjustmentMode::kModern:
      return ModernBrightnessLuts()[index];
    case AdjustmentMode::kLegacy:
      return LegacyBrightnessLuts()[index];
    case AdjustmentMode::kCount:
    default:
      throw AdjustmentInvalidModeError(mode);
  }
}

[[nodiscard]]
const Lut& SelectContrastLut(int value, AdjustmentMode mode) {
  const auto index = static_cast<std::size_t>(value - kContrastMin);

  switch (mode) {
    case AdjustmentMode::kModern:
      return ModernContrastLuts()[index];
    case AdjustmentMode::kLegacy:
      return LegacyContrastLuts()[index];
    case AdjustmentMode::kCount:
    default:
      throw AdjustmentInvalidModeError(mode);
  }
}

void ApplyLutInPlace(Buffer& buffer, const Lut& lut,
                     std::size_t active_channels,
                     parallel::ExecutionPolicy execution) {
  if (buffer.Empty()) {
    return;
  }

  const auto channel_count = buffer.ChannelCount();
  auto* data               = buffer.Data();

  parallel::ForEachIndex(
      execution, std::ptrdiff_t{0},
      static_cast<std::ptrdiff_t>(buffer.PixelCount()), [&](std::ptrdiff_t i) {
        const auto offset = static_cast<std::size_t>(i) * channel_count;
        for (std::size_t channel = 0; channel < active_channels; ++channel) {
          data[offset + channel] = lut[data[offset + channel]];
        }
      });
}

}  // namespace

Buffer AdjustBrightness(const Buffer& input, int value, AdjustmentMode mode,
                        parallel::ExecutionPolicy execution) {
  auto output = input.Clone();
  AdjustBrightnessInPlace(output, value, mode, execution);
  return output;
}

void AdjustBrightnessInPlace(Buffer& buffer, int value, AdjustmentMode mode,
                             parallel::ExecutionPolicy execution) {
  ValidateMode(mode);
  ValidateBrightnessValue(value, mode);
  ValidateAdjustmentColor(buffer.Color());

  ApplyLutInPlace(buffer, SelectBrightnessLut(value, mode),
                  AdjustedChannelCount(buffer.Color()), execution);
}

Buffer AdjustContrast(const Buffer& input, int value, AdjustmentMode mode,
                      parallel::ExecutionPolicy execution) {
  auto output = input.Clone();
  AdjustContrastInPlace(output, value, mode, execution);
  return output;
}

void AdjustContrastInPlace(Buffer& buffer, int value, AdjustmentMode mode,
                           parallel::ExecutionPolicy execution) {
  ValidateMode(mode);
  ValidateContrastValue(value, mode);
  ValidateAdjustmentColor(buffer.Color());

  ApplyLutInPlace(buffer, SelectContrastLut(value, mode),
                  AdjustedChannelCount(buffer.Color()), execution);
}

}  // namespace weqeqq::image
