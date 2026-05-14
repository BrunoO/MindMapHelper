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

/// Extension-first dispatch: `.mmh` → IDocumentRepository; registered extensions → ImportService; others fail.
[[nodiscard]] DocumentPathLoadResult LoadMindMapFromPath(std::string_view path, const IDocumentRepository& repo,
                                                         const ImportService& imports);

}  // namespace mind_map::core
