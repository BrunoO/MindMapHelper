#pragma once

#include <array>
#include <cstddef>
#include <optional>

namespace mind_map::core::mindmap {

inline constexpr int kSampleMindMapNodeCount = 7;

struct SampleMindMapNodeSpec {
  const char* label_;
  std::optional<std::size_t> parent_idx_;  // std::nullopt for the root node
};

// clang-format off
inline constexpr std::array<SampleMindMapNodeSpec, kSampleMindMapNodeCount> kSampleMindMapSpecs = {{
    {"Root",      std::nullopt},
    {"Idea A",    std::size_t{0}},
    {"Idea B",    std::size_t{0}},
    {"Idea C",    std::size_t{0}},
    {"Detail A1", std::size_t{1}},
    {"Detail A2", std::size_t{1}},
    {"Detail B1", std::size_t{2}},
}};
// clang-format on

}  // namespace mind_map::core::mindmap
