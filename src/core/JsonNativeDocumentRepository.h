#pragma once

#include "core/IDocumentRepository.h"

namespace mind_map::core {

/// IDocumentRepository backed by the .mmh JSON format; used by DocumentSessionService for native open/save.
class JsonNativeDocumentRepository final : public IDocumentRepository {
 public:
  [[nodiscard]] std::optional<MindMapDocument> Load(std::string_view path) const override;
  [[nodiscard]] bool Save(std::string_view path, const MindMapDocument& doc) const override;
};

}  // namespace mind_map::core
