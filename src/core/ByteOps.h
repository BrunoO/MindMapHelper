#pragma once

#include <cstdint>

namespace mind_map::core {

/// Bit-casts a char to uint8_t [0, 255]; avoids the signed-char sign-extension pitfall used by Base64 and UTF-8 paths.
[[nodiscard]] constexpr std::uint8_t ToUnsignedByte(char c) noexcept {
  return static_cast<std::uint8_t>(c);
}

}  // namespace mind_map::core
