#include "core/ImportService.h"

#include <cassert>

namespace {

void TestUnknownExtensionReturnsNullopt() {
  const mind_map::core::ImportService service;
  assert(!service.ImportFile("/some/path/file.xyz").has_value());
  assert(!service.ImportFile("/some/path/file.json").has_value());
  assert(!service.ImportFile("/some/path/noextension").has_value());
}

void TestHandlesImportExtension() {
  const mind_map::core::ImportService service;
  assert(service.HandlesImportExtension(".imx"));
  assert(!service.HandlesImportExtension(".IMX"));  // callers must pass lowercased extension
  assert(!service.HandlesImportExtension(".mmh"));
  assert(!service.HandlesImportExtension(""));
}

}  // namespace

int main() {  // NOLINT(bugprone-exception-escape)
  TestUnknownExtensionReturnsNullopt();
  TestHandlesImportExtension();
  return 0;
}
