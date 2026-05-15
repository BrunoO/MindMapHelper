#include "ui/canvas/ClipboardImage.h"

#include "utils/Logger.h"

#include <optional>
#include <string>

namespace mind_map::ui {

// Clipboard image paste is not yet implemented on Linux.
std::optional<std::string> GetClipboardImagePng() {
  static bool logged_once = false;
  if (!logged_once) {
    logged_once = true;
    LOG_INFO_BUILD("GetClipboardImagePng: clipboard image paste is not implemented on Linux");
  }
  return std::nullopt;
}

}  // namespace mind_map::ui
