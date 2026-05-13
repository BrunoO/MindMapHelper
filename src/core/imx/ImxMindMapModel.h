#pragma once

#include <string>
#include <vector>

namespace mind_map::core {

struct ImxNode {
  std::string id_;
  std::string text_;                   // body/element text from the node element itself
  std::string incoming_branch_text_;   // text from the <branch> edge pointing to this node; empty = none
  std::vector<std::string> children_;
  std::string image_asset_id_;  // numeric ID extracted from style="image=<id>;..." attribute
  std::string image_bytes_;     // raw PNG bytes read from ZIP; empty when no image
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
