#include "core/ImportService.h"

#include "core/IImportAdapter.h"
#include "core/PathExtension.h"

#include <string>
#include <string_view>
#include <utility>

namespace mind_map::core {

ImportService::ImportService() {
  adapters_.reserve(4U);
  adapters_.emplace_back(".imx", &imx_adapter_);
}

const IImportAdapter* ImportService::FindAdapterForExtension_(std::string_view lowercase_extension) const {
  for (const auto& [key, adapter] : adapters_) {
    if (key == lowercase_extension) {
      return adapter;
    }
  }
  return nullptr;
}

bool ImportService::HandlesImportExtension(const std::string_view extension) const {
  return FindAdapterForExtension_(LowercaseExtensionOf(extension)) != nullptr;
}

std::optional<MindMapDocument> ImportService::ImportFile(const std::string_view path) const {
  const std::string ext = LowercaseExtensionOf(path);
  if (const IImportAdapter* adapter = FindAdapterForExtension_(ext)) {
    return adapter->Import(path);
  }
  return std::nullopt;
}

}  // namespace mind_map::core
