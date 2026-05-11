#pragma once

#include "core/MindMapDocument.h"

#include <optional>
#include <string_view>

namespace mind_map::core {

class IDocumentRepository {
 public:
  virtual ~IDocumentRepository() = default;
  [[nodiscard]] virtual std::optional<MindMapDocument> Load(std::string_view path) const = 0;
  [[nodiscard]] virtual bool Save(std::string_view path, const MindMapDocument& doc) const = 0;
};

}  // namespace mind_map::core
