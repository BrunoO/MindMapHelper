#include "core/Base64.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace {

void TestEmptyRoundTrip() {
  assert(mind_map::core::Base64Encode("").empty());
  assert(mind_map::core::Base64Decode("").empty());
}

void TestKnownValues() {
  // RFC 4648 test vectors
  assert(mind_map::core::Base64Encode("f") == "Zg==");
  assert(mind_map::core::Base64Encode("fo") == "Zm8=");
  assert(mind_map::core::Base64Encode("foo") == "Zm9v");
  assert(mind_map::core::Base64Encode("foob") == "Zm9vYg==");
  assert(mind_map::core::Base64Encode("fooba") == "Zm9vYmE=");
  assert(mind_map::core::Base64Encode("foobar") == "Zm9vYmFy");
}

void TestRoundTrip() {
  // Build a string with non-ASCII and non-printable bytes (no embedded NUL).
  std::string original;
  original.reserve(16);
  for (int i = 1; i <= 16; ++i) {
    original += static_cast<char>(i * 16 - 1);  // 0x0F, 0x1F, ..., 0xFF
  }
  const std::string encoded = mind_map::core::Base64Encode(original);
  const std::string decoded = mind_map::core::Base64Decode(encoded);
  assert(decoded == original);
}

void TestDecodeKnownValues() {
  assert(mind_map::core::Base64Decode("Zg==") == "f");
  assert(mind_map::core::Base64Decode("Zm8=") == "fo");
  assert(mind_map::core::Base64Decode("Zm9v") == "foo");
  assert(mind_map::core::Base64Decode("Zm9vYmFy") == "foobar");
}

void TestAllByteValues() {
  std::string all_bytes;
  all_bytes.reserve(256);
  for (int i = 0; i < 256; ++i) {
    all_bytes += static_cast<char>(i);
  }
  const std::string encoded = mind_map::core::Base64Encode(all_bytes);
  const std::string decoded = mind_map::core::Base64Decode(encoded);
  assert(decoded == all_bytes);
}

}  // namespace

int main() {  // NOLINT(bugprone-exception-escape)
  TestEmptyRoundTrip();
  TestKnownValues();
  TestRoundTrip();
  TestDecodeKnownValues();
  TestAllByteValues();
  return 0;
}
