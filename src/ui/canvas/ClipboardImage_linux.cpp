#include "ui/canvas/ClipboardImage.h"

#include <optional>
#include <string>

namespace mind_map::ui {

// Clipboard image paste is not yet implemented on Linux.
// X11/Wayland clipboard selection with image/png MIME type requires
// additional integration with the display server. Returning nullopt
// silently disables the feature until a proper implementation is added.
std::optional<std::string> GetClipboardImagePng() {
    return std::nullopt;
}

}  // namespace mind_map::ui
