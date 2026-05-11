#include "core/ImportService.h"

#include <algorithm>
#include <string>
#include <string_view>

namespace mind_map::core {

namespace {

[[nodiscard]] std::string LowercaseExtensionOf(std::string_view path) {
  const auto dot = path.rfind('.');
  if (dot == std::string_view::npos) {
    return {};
  }
  std::string ext{path.substr(dot)};
  std::transform(ext.begin(), ext.end(), ext.begin(),  // NOLINT(llvm-use-ranges)
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return ext;
}

}  // namespace

std::optional<MindMapDocument> ImportService::ImportFile(std::string_view path) const {
  if (LowercaseExtensionOf(path) == ".imx") {
    return imx_adapter_.Import(path);
  }
  return std::nullopt;
}

}  // namespace mind_map::core
