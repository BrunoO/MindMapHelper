#include "ui/canvas/ClipboardImage.h"

#include <optional>
#include <string>

namespace mind_map::ui {

// Clipboard image paste is not yet implemented on Windows.
// The clipboard typically stores images as CF_DIB/CF_DIBV5, which requires
// GDI+ or WIC to encode as PNG. Returning nullopt silently disables the
// feature until a proper implementation is added.
std::optional<std::string> GetClipboardImagePng() {
    return std::nullopt;
}

}  // namespace mind_map::ui
