#pragma once

#include "core/MindMapDocument.h"
#include "core/imx/ImxImportAdapter.h"

#include <optional>
#include <string_view>

namespace mind_map::core {

// Dispatches to the appropriate IImportAdapter based on file extension.
class ImportService {
 public:
  [[nodiscard]] std::optional<MindMapDocument> ImportFile(std::string_view path) const;

 private:
  ImxImportAdapter imx_adapter_;
};

}  // namespace mind_map::core
