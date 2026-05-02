#pragma once

#include <cassert>
#include <cstdint>

namespace mind_map::ui::branch {

enum class BranchStyle : std::uint8_t {
  Bezier = 0,
  Orthogonal = 1,
  OrganicTaper = 2,
};

inline constexpr int kBranchStyleCount = 3;

[[nodiscard]] inline const char* GetBranchStyleDisplayName(BranchStyle style) {
  switch (style) {
    case BranchStyle::Bezier:
      return "Bezier branches (rounded boxes)";
    case BranchStyle::Orthogonal:
      return "Orthogonal polylines (rounded boxes)";
    case BranchStyle::OrganicTaper:
      return "Organic taper (rounded rects)";
    default:
      assert(false && "GetBranchStyleDisplayName: exhaustive switch");
      return "Bezier branches (rounded boxes)";
  }
}

[[nodiscard]] inline BranchStyle BranchStyleFromIndex(int index) {
  if (index <= 0) {
    return BranchStyle::Bezier;
  }
  if (index == 1) {
    return BranchStyle::Orthogonal;
  }
  return BranchStyle::OrganicTaper;
}

}  // namespace mind_map::ui::branch
