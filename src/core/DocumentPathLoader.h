#pragma once

#include "core/MindMapDocument.h"

#include <cstdint>
#include <string_view>

namespace mind_map::core {

class IDocumentRepository;
class ImportService;

enum class DocumentPathLoadOutcome : std::uint8_t { NativeAtPath, ImportedNoPath, Failed };

struct DocumentPathLoadResult {
  DocumentPathLoadOutcome outcome_ = DocumentPathLoadOutcome::Failed;
  MindMapDocument doc_{};
};

// Extension-first dispatch: `.mmh` → native repository; registered import extensions (e.g. `.imx`)
// → ImportService. Other extensions fail without probing file contents.
[[nodiscard]] DocumentPathLoadResult LoadMindMapFromPath(std::string_view path, IDocumentRepository& repo,
                                                         const ImportService& imports);

}  // namespace mind_map::core
