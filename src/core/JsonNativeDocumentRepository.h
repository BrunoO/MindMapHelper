#pragma once

#include "core/IDocumentRepository.h"

namespace mind_map::core {

class JsonNativeDocumentRepository final : public IDocumentRepository {
 public:
  [[nodiscard]] std::optional<MindMapDocument> Load(std::string_view path) const override;
  [[nodiscard]] bool Save(std::string_view path, const MindMapDocument& doc) const override;
};

}  // namespace mind_map::core
