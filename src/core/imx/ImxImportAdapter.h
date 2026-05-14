#pragma once

#include "core/IImportAdapter.h"

namespace mind_map::core {

/// IImportAdapter for .imx: delegates to ImxMindMapLoader, then converts ImxMindMapModel to MindMapDocument.
class ImxImportAdapter final : public IImportAdapter {
 public:
  [[nodiscard]] std::optional<MindMapDocument> Import(std::string_view path) const override;

  // XML-string variant: exposed for testing without constructing a ZIP on disk.
  [[nodiscard]] std::optional<MindMapDocument> ImportFromXml(std::string_view data_xml,
                                                              std::string_view mapmeta_xml) const;
};

}  // namespace mind_map::core
