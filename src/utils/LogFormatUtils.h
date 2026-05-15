#pragma once

#include <array>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <sstream>
#include <string>

/// Thread-safe strerror wrapper for log messages.
inline std::string ThreadSafeStrerror(int errnum) {
  std::array<char, 256> buf{};
#if defined(__APPLE__) || defined(__linux__)
  if (strerror_r(errnum, buf.data(), buf.size()) != 0) {
    return "unknown error";
  }
  return {buf.data()};
#else
  return {std::strerror(errnum)};  // NOLINT(concurrency-mt-unsafe) - no strerror_r on this platform
#endif  // __APPLE__ || __linux__
}

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
