
#pragma once

#include <weqeqq/image/export.h>

#include <format>
#include <source_location>
#include <stdexcept>
namespace weqeqq::image {

struct WQIMAGE_EXPORT Error : std::runtime_error {
  Error(std::string_view message,
        std::source_location location = std::source_location::current())
      : std::runtime_error(std::format("[{}:{}] {}", location.file_name(),
                                       location.line(), message)) {}
};

}  // namespace weqeqq::image
