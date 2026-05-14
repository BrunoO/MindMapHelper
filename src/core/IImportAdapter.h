#pragma once

#include "core/MindMapDocument.h"

#include <optional>
#include <string_view>

namespace mind_map::core {

/// Converts a foreign file format to MindMapDocument; registered in ImportService. ImxImportAdapter handles .imx.
class IImportAdapter {
 public:
  virtual ~IImportAdapter() = default;
  [[nodiscard]] virtual std::optional<MindMapDocument> Import(std::string_view path) const = 0;
};

}  // namespace mind_map::core
