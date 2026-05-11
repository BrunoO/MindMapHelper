#include "core/mindmap/UuidGenerator.h"

#include <cassert>
#include <string>
#include <unordered_set>

namespace {

constexpr size_t kUuidLength   = 36;
constexpr size_t kDash1        = 8;
constexpr size_t kDash2        = 13;
constexpr size_t kDash3        = 18;
constexpr size_t kDash4        = 23;
constexpr size_t kVersionPos   = 14;
constexpr size_t kVariantPos   = 19;
constexpr size_t kUniquenessN  = 1000;

[[nodiscard]] bool IsLowerHexDigit(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

[[nodiscard]] bool IsValidUuidV4(const std::string& uuid) {
  if (uuid.size() != kUuidLength) {
    return false;
  }
  for (const size_t pos : {kDash1, kDash2, kDash3, kDash4}) {
    if (uuid[pos] != '-') {
      return false;
    }
  }
  if (uuid[kVersionPos] != '4') {
    return false;
  }
  const char variant = uuid[kVariantPos];
  if (variant != '8' && variant != '9' && variant != 'a' && variant != 'b') {
    return false;
  }
  for (size_t i = 0; i < kUuidLength; ++i) {
    if (i == kDash1 || i == kDash2 || i == kDash3 || i == kDash4) {
      continue;
    }
    if (!IsLowerHexDigit(uuid[i])) {
      return false;
    }
  }
  return true;
}

void TestFormat() {
  const std::string uuid = mind_map::core::mindmap::GenerateUuidV4();
  assert(IsValidUuidV4(uuid));
}

void TestUniqueness() {
  std::unordered_set<std::string> seen;
  seen.reserve(kUniquenessN);
  for (size_t i = 0; i < kUniquenessN; ++i) {
    const std::string uuid = mind_map::core::mindmap::GenerateUuidV4();
    assert(IsValidUuidV4(uuid));
    assert(seen.insert(uuid).second && "UUID collision detected");
  }
}

}  // namespace

int main() {  // NOLINT(bugprone-exception-escape)
  TestFormat();
  TestUniqueness();
  return 0;
}
