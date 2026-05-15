#include "ui/canvas/MindMapCanvasNodeMutators.h"

#include "core/Base64.h"
#include "ui/MindMapCanvasView.h"
#include "ui/canvas/NodeTextureUtils.h"
#include "utils/Logger.h"

#include <cassert>

namespace mind_map::ui::canvas {

void SetNodeImage(MindMapCanvasView& view, size_t idx, std::string_view png_base64) {
  assert(idx < view.nodes_.size());
  if (idx >= view.nodes_.size()) {
    return;
  }
  auto& node = view.nodes_[idx];
  ReleaseTexture(node.texture_id_);
  node.texture_id_ = 0;
  node.image_png_base64_ = png_base64;
  if (!png_base64.empty()) {
    const std::string png_bytes = mind_map::core::Base64Decode(node.image_png_base64_);
    if (png_bytes.empty()) {
      LOG_WARNING_BUILD("SetNodeImage: base64 decode produced empty PNG for node " << node.id_);
    }
    node.texture_id_ = UploadPngTexture(png_bytes, "node " + node.id_);
  }
}

const std::string& GetNodeImageBase64(const MindMapCanvasView& view, size_t idx) {
  static const std::string kEmpty;
  assert(idx < view.nodes_.size());
  if (idx >= view.nodes_.size()) {
    return kEmpty;
  }
  return view.nodes_[idx].image_png_base64_;
}

void SetNodeLabel(MindMapCanvasView& view, size_t idx, std::string_view label) {
  assert(idx < view.nodes_.size());
  if (idx >= view.nodes_.size()) {
    return;
  }
  view.nodes_[idx].label_ = label;
}

const std::string& GetNodeLabel(const MindMapCanvasView& view, size_t idx) {
  static const std::string kEmpty;
  assert(idx < view.nodes_.size());
  if (idx >= view.nodes_.size()) {
    return kEmpty;
  }
  return view.nodes_[idx].label_;
}

void SetEdgeLabel(MindMapCanvasView& view, size_t idx, std::string_view label) {
  assert(idx < view.nodes_.size());
  if (idx >= view.nodes_.size()) {
    return;
  }
  view.nodes_[idx].branch_edge_label_ = label;
}

const std::string& GetEdgeLabel(const MindMapCanvasView& view, size_t idx) {
  static const std::string kEmpty;
  assert(idx < view.nodes_.size());
  if (idx >= view.nodes_.size()) {
    return kEmpty;
  }
  return view.nodes_[idx].branch_edge_label_;
}

}  // namespace mind_map::ui::canvas
