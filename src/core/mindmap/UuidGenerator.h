#pragma once

#include <string>

namespace mind_map::core::mindmap {

/// Returns a random UUID v4 string; used to generate MindMapNode::id_ and MindMapEdge::id_.
[[nodiscard]] std::string GenerateUuidV4();

}  // namespace mind_map::core::mindmap
