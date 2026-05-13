#pragma once

#include "core/ByteOps.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace mind_map::core {

namespace base64_detail {

constexpr std::string_view kAlphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
constexpr uint32_t kBitsPerB64Char = 6U;
constexpr uint32_t kB64Mask        = 0x3FU;   // 6-bit mask
constexpr uint32_t kByteMask       = 0xFFU;   // 8-bit mask
constexpr uint8_t  kInvalidEntry   = 0xFFU;   // decode-table sentinel for invalid chars

}  // namespace base64_detail

[[nodiscard]] inline std::string Base64Encode(std::string_view raw) {
  using namespace base64_detail;  // NOLINT(google-build-using-namespace)
  constexpr uint32_t kShift0 = kBitsPerB64Char * 3U;  // 18
  constexpr uint32_t kShift1 = kBitsPerB64Char * 2U;  // 12
  constexpr uint32_t kShift2 = kBitsPerB64Char * 1U;  // 6

  std::string out;
  out.reserve(((raw.size() + 2U) / 3U) * 4U);
  for (size_t i = 0; i < raw.size(); i += 3U) {
    const std::uint32_t b0 = ToUnsignedByte(raw[i]);
    const std::uint32_t b1 = (i + 1U < raw.size()) ? ToUnsignedByte(raw[i + 1U]) : 0U;
    const std::uint32_t b2 = (i + 2U < raw.size()) ? ToUnsignedByte(raw[i + 2U]) : 0U;
    const uint32_t triple = (b0 << 16U) | (b1 << 8U) | b2;
    out += kAlphabet[(triple >> kShift0) & kB64Mask];
    out += kAlphabet[(triple >> kShift1) & kB64Mask];
    out += (i + 1U < raw.size()) ? kAlphabet[(triple >> kShift2) & kB64Mask] : '=';
    out += (i + 2U < raw.size()) ? kAlphabet[triple & kB64Mask] : '=';
  }
  return out;
}

[[nodiscard]] inline std::string Base64Decode(std::string_view encoded) {
  using namespace base64_detail;  // NOLINT(google-build-using-namespace)
  static const auto kDecTable = [] {
    std::array<uint8_t, 256> t{};
    t.fill(kInvalidEntry);
    for (uint8_t i = 0; i < 64U; ++i) {
      t[ToUnsignedByte(kAlphabet[i])] = i;
    }
    return t;
  }();

  std::string out;
  out.reserve((encoded.size() / 4U) * 3U);
  uint32_t accumulator = 0U;
  int bits = 0;
  for (const char c : encoded) {
    if (c == '=') {
      break;
    }
    const uint8_t val = kDecTable[ToUnsignedByte(c)];
    if (val == kInvalidEntry) {
      continue;  // skip whitespace or invalid chars
    }
    accumulator = (accumulator << kBitsPerB64Char) | val;
    bits += static_cast<int>(kBitsPerB64Char);
    if (bits >= 8) {
      bits -= 8;
      out += static_cast<char>((accumulator >> static_cast<uint32_t>(bits)) & kByteMask);
    }
  }
  return out;
}

}  // namespace mind_map::core
