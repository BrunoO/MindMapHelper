#pragma once

#include <cstdint>

namespace mind_map::core {

// Reads a `char` as its unsigned-byte value [0, 255]. Centralises the
// `static_cast<std::uint8_t>(c)` idiom used in byte-arithmetic code paths
// (Base64, UTF-8 decoding, etc.) and avoids the signed-char sign-extension
// pitfall when `char` is signed on the host platform.
//
// Widening to wider unsigned integers is implicit and lossless at the call
// site, e.g. `const std::uint32_t b = ToUnsignedByte(s[i]);`.
[[nodiscard]] constexpr std::uint8_t ToUnsignedByte(char c) noexcept {
  return static_cast<std::uint8_t>(c);
}

}  // namespace mind_map::core
