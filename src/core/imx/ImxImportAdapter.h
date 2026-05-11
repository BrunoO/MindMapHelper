#pragma once

#include "core/IImportAdapter.h"

namespace mind_map::core {

class ImxImportAdapter final : public IImportAdapter {
 public:
  [[nodiscard]] std::optional<MindMapDocument> Import(std::string_view path) const override;
};

}  // namespace mind_map::core
