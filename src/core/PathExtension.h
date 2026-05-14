#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace mind_map::core {

/// Native on-disk format used by JsonNativeDocumentRepository and DocumentPathLoader for extension dispatch.
inline constexpr std::string_view kNativeMindMapFileExtension = ".mmh";

[[nodiscard]] inline std::string LowercaseExtensionOf(std::string_view path) {
  const auto dot = path.rfind('.');
  if (dot == std::string_view::npos) {
    return {};
  }
  std::string ext{path.substr(dot)};
  std::transform(ext.begin(), ext.end(), ext.begin(),  // NOLINT(llvm-use-ranges)
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return ext;
}

}  // namespace mind_map::core
