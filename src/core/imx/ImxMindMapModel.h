#pragma once

#include <string>
#include <vector>

namespace mind_map::core {

struct ImxNode {
  std::string id_;
  std::string text_;
  std::vector<std::string> children_;
};

struct ImxMapMeta {
  std::string title_;
  std::string author_;
  std::string created_;
};

/// Parsed IMX mind map (ZIP + `data.xml` / `mapmeta.xml`) before conversion to `MindMapDocument`.
struct ImxMindMapModel {
  std::string root_id_;
  std::vector<ImxNode> nodes_;
  ImxMapMeta meta_;
};

}  // namespace mind_map::core
