#pragma once

#include <optional>
#include <string>

namespace mind_map::ui {

// Returns raw PNG bytes from the system clipboard image, or nullopt if no
// image is present. Thread-safe on macOS; must be called from the main thread
// on Windows and Linux.
[[nodiscard]] std::optional<std::string> GetClipboardImagePng();

}  // namespace mind_map::ui
