#pragma once

#include "core/MindMapDocument.h"

#include <optional>
#include <string_view>

namespace mind_map::core {

class IImportAdapter {
 public:
  virtual ~IImportAdapter() = default;
  [[nodiscard]] virtual std::optional<MindMapDocument> Import(std::string_view path) const = 0;
};

}  // namespace mind_map::core
