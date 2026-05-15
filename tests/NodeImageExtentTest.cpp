#include "ui/canvas/NodeGeometry.h"

#include <cassert>
#include <cmath>

namespace {

constexpr float kFloatTolerance = 0.01F;
constexpr int kSquareImagePx = 100;
constexpr float kSquareImageExpectedHalf = 50.0F;
constexpr int kWideImageWidthPx = 200;
constexpr int kWideImageHeightPx = 100;
constexpr float kWideImageAspectRatio = 2.0F;
constexpr int kLargeImageWidthPx = 4000;
constexpr int kLargeImageHeightPx = 2000;

void TestSquareImageExtent() {
  const ImVec2 half =
      mind_map::canvas::NodeHalfExtentForImagePixels(kSquareImagePx, kSquareImagePx);
  assert(std::abs(half.x - half.y) < kFloatTolerance);
  assert(std::abs(half.x - kSquareImageExpectedHalf) < kFloatTolerance);
}

void TestRectangularImageExtentPreservesAspect() {
  const ImVec2 half = mind_map::canvas::NodeHalfExtentForImagePixels(kWideImageWidthPx,
                                                                     kWideImageHeightPx);
  assert(std::abs((half.x / half.y) - kWideImageAspectRatio) < kFloatTolerance);
}

void TestLargeImageIsScaledDown() {
  const ImVec2 half = mind_map::canvas::NodeHalfExtentForImagePixels(kLargeImageWidthPx,
                                                                     kLargeImageHeightPx);
  const float max_half = mind_map::canvas::kNodeMaxLabelWidth * 0.5F + mind_map::canvas::kNodePad;
  assert(half.x <= max_half + kFloatTolerance);
  assert(half.y <= max_half + kFloatTolerance);
  assert(std::abs((half.x / half.y) - kWideImageAspectRatio) < kFloatTolerance);
}

}  // namespace

int main() {  // NOLINT(bugprone-exception-escape)
  TestSquareImageExtent();
  TestRectangularImageExtentPreservesAspect();
  TestLargeImageIsScaledDown();
  return 0;
}
