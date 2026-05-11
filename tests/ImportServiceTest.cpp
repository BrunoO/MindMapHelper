#include "core/ImportService.h"

#include <cassert>

namespace {

void TestUnknownExtensionReturnsNullopt() {
  const mind_map::core::ImportService service;
  assert(!service.ImportFile("/some/path/file.xyz").has_value());
  assert(!service.ImportFile("/some/path/file.json").has_value());
  assert(!service.ImportFile("/some/path/noextension").has_value());
}

}  // namespace

int main() {  // NOLINT(bugprone-exception-escape)
  TestUnknownExtensionReturnsNullopt();
  return 0;
}
