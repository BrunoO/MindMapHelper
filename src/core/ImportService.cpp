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

const IImportAdapter* ImportService::FindAdapterForExtension_(const std::string& lowercase_extension) const {
  for (const auto& entry : adapters_) {
    if (entry.first == lowercase_extension) {
      return entry.second;
    }
  }
  return nullptr;
}

bool ImportService::HandlesImportExtension(const std::string_view lowercase_extension) const {
  const std::string ext{lowercase_extension};
  return FindAdapterForExtension_(ext) != nullptr;
}

std::optional<MindMapDocument> ImportService::ImportFile(const std::string_view path) const {
  const std::string ext = LowercaseExtensionOf(path);
  if (const IImportAdapter* adapter = FindAdapterForExtension_(ext)) {
    return adapter->Import(path);
  }
  return std::nullopt;
}

}  // namespace mind_map::core
