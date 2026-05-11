#include "core/ImportService.h"

#include <string_view>

namespace mind_map::core {

namespace {

[[nodiscard]] std::string_view ExtensionOf(std::string_view path) {
  const auto dot = path.rfind('.');
  return dot != std::string_view::npos ? path.substr(dot) : std::string_view{};
}

}  // namespace

std::optional<MindMapDocument> ImportService::ImportFile(std::string_view path) const {
  const std::string_view ext = ExtensionOf(path);
  if (ext == ".imx" || ext == ".IMX") {
    return imx_adapter_.Import(path);
  }
  return std::nullopt;
}

}  // namespace mind_map::core
