#include "core/mindmap/UuidGenerator.h"

#include <array>
#include <cstdint>
#include <random>
#include <sstream>

namespace mind_map::core::mindmap {

namespace {

constexpr std::uint32_t kU32Max        = 0xFFFFFFFFU;
constexpr std::uint32_t kVersionMask   = 0xFFFF0FFFU;  // clear bits 76-79
constexpr std::uint32_t kVersionBits   = 0x00004000U;  // version = 4
constexpr std::uint32_t kVariantMask   = 0x3FFFFFFFU;  // clear bits 62-63
constexpr std::uint32_t kVariantBits   = 0x80000000U;  // variant = 10xx
constexpr std::uint32_t kLowWordMask   = 0x0000FFFFU;
constexpr unsigned int  kHighWordShift = 16U;

}  // namespace

std::string GenerateUuidV4() {
  // Each call gets its own seeded engine; no shared mutable state.
  std::mt19937 rng{std::random_device{}()};
  std::uniform_int_distribution<std::uint32_t> dist32(0, kU32Max);

  // Fill 128 bits as four 32-bit words.
  std::array<std::uint32_t, 4> b{dist32(rng), dist32(rng), dist32(rng), dist32(rng)};

  // Set version 4 bits (bits 76-79) and variant bits (bits 62-63).
  b[1] = (b[1] & kVersionMask) | kVersionBits;
  b[2] = (b[2] & kVariantMask) | kVariantBits;

  // Format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
  std::ostringstream oss;
  oss << std::hex;
  auto hex8 = [&](std::uint32_t v) {
    oss.width(8);
    oss.fill('0');
    oss << v;
  };
  auto hex4 = [&](std::uint32_t v) {
    oss.width(4);
    oss.fill('0');
    oss << v;
  };

  hex8(b[0]);
  oss << '-';
  hex4(b[1] >> kHighWordShift);
  oss << '-';
  hex4(b[1] & kLowWordMask);
  oss << '-';
  hex4(b[2] >> kHighWordShift);
  oss << '-';
  hex4(b[2] & kLowWordMask);
  hex8(b[3]);

  return oss.str();
}

}  // namespace mind_map::core::mindmap
