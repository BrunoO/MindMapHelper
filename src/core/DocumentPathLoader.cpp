#include "core/DocumentPathLoader.h"

#include "core/IDocumentRepository.h"
#include "core/ImportService.h"
#include "core/PathExtension.h"

#include <iostream>
#include <utility>

namespace mind_map::core {

DocumentPathLoadResult LoadMindMapFromPath(std::string_view path, IDocumentRepository& repo,
                                           const ImportService& imports) {
  const std::string ext = LowercaseExtensionOf(path);
  if (ext == kNativeMindMapFileExtension) {
    if (auto native = repo.Load(path)) {
      return {DocumentPathLoadOutcome::NativeAtPath, std::move(*native)};
    }
    std::cerr << "LoadMindMapFromPath: failed to load native document '" << path << "'\n";
    return {};
  }

  if (imports.HandlesImportExtension(ext)) {
    if (auto imported = imports.ImportFile(path)) {
      return {DocumentPathLoadOutcome::ImportedNoPath, std::move(*imported)};
    }
    std::cerr << "LoadMindMapFromPath: import failed for '" << path << "'\n";
    return {};
  }

  std::cerr << "LoadMindMapFromPath: unsupported extension for '" << path << "'\n";
  return {};
}

}  // namespace mind_map::core
