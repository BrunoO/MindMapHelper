#pragma once

#include <cstddef>
#include <sstream>
#include <string>

/// Human-readable byte size for log lines (e.g. "12 MB").
inline std::string FormatMemory(size_t bytes) {
  if (bytes == 0) {
    return "0 B";
  }

  const size_t kKB = 1024;
  const size_t kMB = kKB * 1024;
  const size_t kGB = kMB * 1024;

  std::ostringstream oss;

  if (bytes >= kGB) {
    const size_t gb = (bytes + (kGB / 2)) / kGB;
    oss << gb << " GB";
  } else if (bytes >= kMB) {
    const size_t mb = (bytes + (kMB / 2)) / kMB;
    oss << mb << " MB";
  } else if (bytes >= kKB) {
    const size_t kb = (bytes + (kKB / 2)) / kKB;
    oss << kb << " KB";
  } else {
    oss << bytes << " B";
  }

  return oss.str();
}
