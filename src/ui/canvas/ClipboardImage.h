#pragma once

#include <optional>
#include <string>

namespace mind_map::ui {

/// Returns raw PNG bytes from the clipboard image, or nullopt if none. Main thread on Windows/Linux; thread-safe on macOS.
[[nodiscard]] std::optional<std::string> GetClipboardImagePng();

}  // namespace mind_map::ui
