module;

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <vector>

export module weqeqq.image.processing:resize;

import :error;
export import weqeqq.image;
export import weqeqq.parallel;

export namespace weqeqq::image {

enum class Filter {
  Nearest,
  Triangle,
  CatmullRom,
  Mitchell,
  Lanczos3,
  Box,
  Lanczos2,
  Hamming,
};

enum class Gamma {
  kLinear,
  kSrgb,
};

}  // namespace weqeqq::image

namespace weqeqq::image {

namespace internal {

inline double Sinc(double x) {
  if (x == 0.0) {
    return 1.0;
  }
  x *= std::numbers::pi_v<double>;
  return std::sin(x) / x;
}

inline double BoxKernel(double x) {
  return (x >= -0.5 && x < 0.5) ? 1.0 : 0.0;
}

inline double TriangleKernel(double x) {
  x = std::abs(x);
  return x < 1.0 ? 1.0 - x : 0.0;
}

inline double CubicKernel(double x, double b, double c) {
  x             = std::abs(x);
  const auto x2 = x * x;
  const auto x3 = x2 * x;
  if (x < 1.0) {
    return ((12.0 - 9.0 * b - 6.0 * c) * x3 +
            (-18.0 + 12.0 * b + 6.0 * c) * x2 + (6.0 - 2.0 * b)) /
           6.0;
  }
  if (x < 2.0) {
    return ((-b - 6.0 * c) * x3 + (6.0 * b + 30.0 * c) * x2 +
            (-12.0 * b - 48.0 * c) * x + (8.0 * b + 24.0 * c)) /
           6.0;
  }
  return 0.0;
}

inline double CatmullRomKernel(double x) { return CubicKernel(x, 0.0, 0.5); }

inline double MitchellKernel(double x) {
  return CubicKernel(x, 1.0 / 3.0, 1.0 / 3.0);
}

inline double LanczosKernel(double x, double a) {
  x = std::abs(x);
  return x < a ? Sinc(x) * Sinc(x / a) : 0.0;
}

inline double Lanczos2Kernel(double x) { return LanczosKernel(x, 2.0); }
inline double Lanczos3Kernel(double x) { return LanczosKernel(x, 3.0); }

inline double HammingKernel(double x) {
  x = std::abs(x);
  if (x >= 1.0) {
    return 0.0;
  }
  const auto window = 0.54 + 0.46 * std::cos(std::numbers::pi_v<double> * x);
  return Sinc(x) * window;
}

struct FilterSpec {
  double (*kernel)(double) = nullptr;
  double support           = 0.0;
  bool nearest             = false;
};

[[noreturn]] void ThrowInvalidFilter(Filter filter);

[[nodiscard]] FilterSpec GetFilterSpec(Filter filter) {
  switch (filter) {
    case Filter::Nearest:
      return {nullptr, 0.5, true};
    case Filter::Triangle:
      return {TriangleKernel, 1.0, false};
    case Filter::CatmullRom:
      return {CatmullRomKernel, 2.0, false};
    case Filter::Mitchell:
      return {MitchellKernel, 2.0, false};
    case Filter::Lanczos3:
      return {Lanczos3Kernel, 3.0, false};
    case Filter::Box:
      return {BoxKernel, 0.5, false};
    case Filter::Lanczos2:
      return {Lanczos2Kernel, 2.0, false};
    case Filter::Hamming:
      return {HammingKernel, 1.0, false};
    default:
      ThrowInvalidFilter(filter);
  }
}

struct AxisContributions {
  std::size_t window = 0;
  std::vector<std::ptrdiff_t> starts;
  std::vector<float> weights;
};

[[nodiscard]] AxisContributions BuildContributions(std::size_t src_len,
                                                    std::size_t dst_len,
                                                    const FilterSpec& spec) {
  const auto scale = static_cast<double>(dst_len) / static_cast<double>(src_len);
  const auto filter_scale = scale < 1.0 ? 1.0 / scale : 1.0;

  AxisContributions c;

  if (spec.nearest) {
    c.window = 1;
    c.starts.resize(dst_len);
    c.weights.assign(dst_len, 1.0f);
    for (std::size_t i = 0; i < dst_len; ++i) {
      const auto center = (static_cast<double>(i) + 0.5) / scale - 0.5;
      auto src          = static_cast<std::ptrdiff_t>(std::floor(center + 0.5));
      src = std::clamp<std::ptrdiff_t>(
          src, 0, static_cast<std::ptrdiff_t>(src_len) - 1);
      c.starts[i] = src;
    }
    return c;
  }

  const auto support = spec.support * filter_scale;
  const auto window  = static_cast<std::size_t>(std::ceil(support * 2.0)) + 1;

  c.window = window;
  c.starts.resize(dst_len);
  c.weights.assign(dst_len * window, 0.0f);

  for (std::size_t i = 0; i < dst_len; ++i) {
    const auto center = (static_cast<double>(i) + 0.5) / scale - 0.5;
    const auto left   = static_cast<std::ptrdiff_t>(std::floor(center - support));
    const auto right  = static_cast<std::ptrdiff_t>(std::ceil(center + support));

    c.starts[i] = left;

    double sum = 0.0;
    for (auto s = left; s <= right; ++s) {
      const auto k = static_cast<std::size_t>(s - left);
      if (k >= window) {
        break;
      }
      const auto arg = (static_cast<double>(s) - center) / filter_scale;
      const auto w   = spec.kernel(arg);
      c.weights[i * window + k] = static_cast<float>(w);
      sum += w;
    }
    if (sum != 0.0) {
      for (std::size_t k = 0; k < window; ++k) {
        c.weights[i * window + k] /= static_cast<float>(sum);
      }
    }
  }
  return c;
}

[[nodiscard]] const std::array<float, 256>& SrgbToLinearLut() {
  static const auto lut = [] {
    std::array<float, 256> table{};
    for (int i = 0; i < 256; ++i) {
      const auto c = static_cast<double>(i) / 255.0;
      table[static_cast<std::size_t>(i)] = static_cast<float>(
          c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4));
    }
    return table;
  }();
  return lut;
}

[[nodiscard]] float LinearToSrgb(float linear) {
  const auto c = static_cast<double>(linear);
  const auto s = c <= 0.0031308
                     ? c * 12.92
                     : 1.055 * std::pow(c, 1.0 / 2.4) - 0.055;
  return static_cast<float>(s);
}

[[nodiscard]] std::size_t GammaChannelCount(Color color) {
  switch (color) {
    case Color::kGrayscale:
      return 1;
    case Color::kRgb:
    case Color::kRgba:
      return 3;
    case Color::kCmyk:
    default:
      return 0;
  }
}

[[noreturn]] void ThrowInvalidFilter(Filter filter) {
  throw error::TypedErrorBuilder(ProcessingError::kResizeInvalidFilter,
                                 "resize filter is invalid")
      .Field("filter", static_cast<int>(filter))
      .Details("The supplied filter is not a supported enum value.")
      .Hint("Use one of the weqeqq::image::Filter enumerators.")
      .Build();
}

[[noreturn]] void ThrowEmptyInput() {
  throw error::TypedErrorBuilder(ProcessingError::kResizeEmptyInput,
                                 "resize input buffer is empty")
      .Details("Resize requires a buffer with at least one pixel.")
      .Hint("Pass a non-empty source buffer.")
      .Build();
}

[[noreturn]] void ThrowInvalidDimensions(std::size_t width,
                                         std::size_t height) {
  throw error::TypedErrorBuilder(ProcessingError::kResizeInvalidDimensions,
                                 "resize target dimensions are invalid")
      .Field("width", width)
      .Field("height", height)
      .Details("Resize dimensions must be positive.")
      .Hint("Pass non-zero new_width and new_height.")
      .Build();
}

[[noreturn]] void ThrowInvalidScale(double scale_x, double scale_y) {
  throw error::TypedErrorBuilder(ProcessingError::kResizeInvalidScale,
                                 "resize scale factor is invalid")
      .Field("scale_x", scale_x)
      .Field("scale_y", scale_y)
      .Details("Scale factors must be finite and greater than zero.")
      .Hint("Pass positive, finite scale factors.")
      .Build();
}

void ResampleHorizontal(const std::vector<float>& src, std::vector<float>& dst,
                        std::size_t src_w, std::size_t height,
                        std::size_t dst_w, std::size_t channels,
                        const AxisContributions& contrib,
                        parallel::ExecutionPolicy execution) {
  const auto window     = contrib.window;
  const auto src_w_diff = static_cast<std::ptrdiff_t>(src_w) - 1;

  parallel::ForEachIndex(
      execution, std::ptrdiff_t{0}, static_cast<std::ptrdiff_t>(height),
      [&](std::ptrdiff_t row_index) {
        const auto y       = static_cast<std::size_t>(row_index);
        const auto src_row = y * src_w * channels;
        const auto dst_row = y * dst_w * channels;
        for (std::size_t x = 0; x < dst_w; ++x) {
          const auto base = contrib.starts[x];
          const auto wbeg = x * window;
          auto* out       = &dst[dst_row + x * channels];
          for (std::size_t ch = 0; ch < channels; ++ch) {
            out[ch] = 0.0f;
          }
          for (std::size_t k = 0; k < window; ++k) {
            const auto w = contrib.weights[wbeg + k];
            if (w == 0.0f) {
              continue;
            }
            const auto sx = std::clamp<std::ptrdiff_t>(
                base + static_cast<std::ptrdiff_t>(k), 0, src_w_diff);
            const auto* in =
                &src[src_row + static_cast<std::size_t>(sx) * channels];
            for (std::size_t ch = 0; ch < channels; ++ch) {
              out[ch] += w * in[ch];
            }
          }
        }
      });
}

void ResampleVertical(const std::vector<float>& src, std::vector<float>& dst,
                      std::size_t width, std::size_t src_h, std::size_t dst_h,
                      std::size_t channels, const AxisContributions& contrib,
                      parallel::ExecutionPolicy execution) {
  const auto window     = contrib.window;
  const auto row_stride = width * channels;
  const auto src_h_diff = static_cast<std::ptrdiff_t>(src_h) - 1;

  parallel::ForEachIndex(
      execution, std::ptrdiff_t{0}, static_cast<std::ptrdiff_t>(dst_h),
      [&](std::ptrdiff_t row_index) {
        const auto y    = static_cast<std::size_t>(row_index);
        const auto base = contrib.starts[y];
        const auto wbeg = y * window;
        auto* out_row   = &dst[y * row_stride];
        for (std::size_t i = 0; i < row_stride; ++i) {
          out_row[i] = 0.0f;
        }
        for (std::size_t k = 0; k < window; ++k) {
          const auto w = contrib.weights[wbeg + k];
          if (w == 0.0f) {
            continue;
          }
          const auto sy = std::clamp<std::ptrdiff_t>(
              base + static_cast<std::ptrdiff_t>(k), 0, src_h_diff);
          const auto* in_row = &src[static_cast<std::size_t>(sy) * row_stride];
          for (std::size_t i = 0; i < row_stride; ++i) {
            out_row[i] += w * in_row[i];
          }
        }
      });
}

[[nodiscard]] Buffer ResizeImpl(const Buffer& input, std::size_t new_width,
                                std::size_t new_height, Filter filter,
                                Gamma gamma,
                                parallel::ExecutionPolicy execution) {
  if (input.Empty()) {
    ThrowEmptyInput();
  }
  if (new_width == 0 || new_height == 0) {
    ThrowInvalidDimensions(new_width, new_height);
  }

  const auto spec     = GetFilterSpec(filter);
  const auto color    = input.Color();
  const auto channels = input.ChannelCount();
  const auto src_w    = input.Width();
  const auto src_h    = input.Height();

  const auto linearize   = gamma == Gamma::kSrgb;
  const auto gamma_count = linearize ? GammaChannelCount(color) : 0;
  const auto has_alpha   = color == Color::kRgba;
  const auto alpha_index = std::size_t{3};

  if (src_w == new_width && src_h == new_height) {
    return input.Clone();
  }

  const auto& to_linear = SrgbToLinearLut();
  const auto* src_data  = input.Data();

  std::vector<float> src_f(src_w * src_h * channels);
  parallel::ForEachIndex(
      execution, std::ptrdiff_t{0},
      static_cast<std::ptrdiff_t>(src_w * src_h), [&](std::ptrdiff_t pixel) {
        const auto offset = static_cast<std::size_t>(pixel) * channels;
        float alpha       = 1.0f;
        if (has_alpha) {
          alpha = static_cast<float>(src_data[offset + alpha_index]) / 255.0f;
        }
        for (std::size_t ch = 0; ch < channels; ++ch) {
          const auto value = src_data[offset + ch];
          float v;
          if (has_alpha && ch == alpha_index) {
            v = alpha;
          } else if (ch < gamma_count) {
            v = to_linear[value];
            if (has_alpha) {
              v *= alpha;
            }
          } else {
            v = static_cast<float>(value) / 255.0f;
            if (has_alpha) {
              v *= alpha;
            }
          }
          src_f[offset + ch] = v;
        }
      });

  const auto col_contrib = BuildContributions(src_w, new_width, spec);
  const auto row_contrib = BuildContributions(src_h, new_height, spec);

  std::vector<float> mid_f(new_width * src_h * channels);
  ResampleHorizontal(src_f, mid_f, src_w, src_h, new_width, channels,
                     col_contrib, execution);

  std::vector<float> dst_f(new_width * new_height * channels);
  ResampleVertical(mid_f, dst_f, new_width, src_h, new_height, channels,
                   row_contrib, execution);

  Buffer output(new_width, new_height, color);
  auto* out_data = output.Data();
  parallel::ForEachIndex(
      execution, std::ptrdiff_t{0},
      static_cast<std::ptrdiff_t>(new_width * new_height),
      [&](std::ptrdiff_t pixel) {
        const auto offset = static_cast<std::size_t>(pixel) * channels;
        float inv_alpha   = 0.0f;
        if (has_alpha) {
          const auto alpha = dst_f[offset + alpha_index];
          inv_alpha        = alpha > 0.0f ? 1.0f / alpha : 0.0f;
        }
        for (std::size_t ch = 0; ch < channels; ++ch) {
          float v = dst_f[offset + ch];
          if (has_alpha && ch != alpha_index) {
            v *= inv_alpha;
          }
          if (ch < gamma_count) {
            v = LinearToSrgb(v);
          }
          const auto scaled = std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0);
          out_data[offset + ch] = static_cast<std::uint8_t>(scaled);
        }
      });

  return output;
}

}  // namespace internal

export [[nodiscard]] Buffer Resize(
    const Buffer& input, std::size_t new_width, std::size_t new_height,
    Filter filter = Filter::Lanczos3, Gamma gamma = Gamma::kSrgb,
    parallel::ExecutionPolicy execution = parallel::Execution::kSequential) {
  return internal::ResizeImpl(input, new_width, new_height, filter, gamma,
                              execution);
}

export [[nodiscard]] Buffer ResizeByScale(
    const Buffer& input, double scale_x, double scale_y,
    Filter filter = Filter::Lanczos3, Gamma gamma = Gamma::kSrgb,
    parallel::ExecutionPolicy execution = parallel::Execution::kSequential) {
  if (!std::isfinite(scale_x) || !std::isfinite(scale_y) || scale_x <= 0.0 ||
      scale_y <= 0.0) {
    internal::ThrowInvalidScale(scale_x, scale_y);
  }
  const auto new_width = std::max<std::size_t>(
      1, static_cast<std::size_t>(
             std::llround(static_cast<double>(input.Width()) * scale_x)));
  const auto new_height = std::max<std::size_t>(
      1, static_cast<std::size_t>(
             std::llround(static_cast<double>(input.Height()) * scale_y)));
  return internal::ResizeImpl(input, new_width, new_height, filter, gamma,
                              execution);
}

export [[nodiscard]] Buffer ResizeByScale(
    const Buffer& input, double scale, Filter filter = Filter::Lanczos3,
    Gamma gamma                         = Gamma::kSrgb,
    parallel::ExecutionPolicy execution = parallel::Execution::kSequential) {
  return ResizeByScale(input, scale, scale, filter, gamma, execution);
}

}  // namespace weqeqq::image
