#pragma once

#include <array>

namespace mind_map::core::mindmap {

inline constexpr int kSampleMindMapNodeCount = 7;

struct SampleMindMapNodeSpec {
  const char* label_;
  int parent_;
};

extern const std::array<SampleMindMapNodeSpec, kSampleMindMapNodeCount> kSampleMindMapSpecs;

}  // namespace mind_map::core::mindmap
