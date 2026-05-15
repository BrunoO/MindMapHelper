#include "core/DocumentPathLoader.h"

#include "core/IDocumentRepository.h"
#include "core/ImportService.h"
#include "core/PathExtension.h"
#include "utils/Logger.h"

#include <utility>

namespace mind_map::core {

DocumentPathLoadResult LoadMindMapFromPath(std::string_view path, const IDocumentRepository& repo,
                                           const ImportService& imports) {
  const std::string ext = LowercaseExtensionOf(path);
  if (ext == kNativeMindMapFileExtension) {
    if (auto native = repo.Load(path)) {
      return {DocumentPathLoadOutcome::NativeAtPath, std::move(*native)};
    }
    LOG_ERROR_BUILD("LoadMindMapFromPath: failed to load native document '" << path << '\'');
    return {};
  }

  if (imports.HandlesImportExtension(ext)) {
    if (auto imported = imports.ImportFile(path)) {
      return {DocumentPathLoadOutcome::ImportedNoPath, std::move(*imported)};
    }
    LOG_ERROR_BUILD("LoadMindMapFromPath: import failed for '" << path << '\'');
    return {};
  }

  LOG_ERROR_BUILD("LoadMindMapFromPath: unsupported extension for '" << path << '\'');
  return {};
}

}  // namespace mind_map::core
