#pragma once

#include "core/MindMapDocument.h"
#include "core/imx/ImxImportAdapter.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mind_map::core {

class IImportAdapter;

/// Dispatches to the registered IImportAdapter for a file extension; pre-registers ImxImportAdapter for .imx.
class ImportService {
 public:
  ImportService();

  [[nodiscard]] std::optional<MindMapDocument> ImportFile(std::string_view path) const;

  /// Returns true if a registered adapter handles @p extension (case-insensitive, with leading dot).
  [[nodiscard]] bool HandlesImportExtension(std::string_view extension) const;

 private:
  [[nodiscard]] const IImportAdapter* FindAdapterForExtension_(std::string_view lowercase_extension) const;

  ImxImportAdapter imx_adapter_;
  std::vector<std::pair<std::string, const IImportAdapter*>> adapters_;
};

}  // namespace mind_map::core
