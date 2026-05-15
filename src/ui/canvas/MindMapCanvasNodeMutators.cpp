#include "ui/canvas/MindMapCanvasNodeMutators.h"

#include "core/Base64.h"
#include "ui/MindMapCanvasView.h"
#include "ui/canvas/CanvasNode.h"
#include "ui/canvas/NodeGeometry.h"
#include "ui/canvas/NodeTextureUtils.h"
#include "utils/Logger.h"

#include <cassert>
#include <string>

namespace mind_map::ui::canvas {

void ApplyImageToCanvasNode(CanvasNode& node, std::string_view png_base64) {
  ReleaseTexture(node.texture_id_);
  node.texture_id_ = 0;
  node.image_png_base64_.assign(png_base64);
  if (png_base64.empty()) {
    node.half_extent_override_ = {0.0F, 0.0F};
    return;
  }
  const std::string png_bytes = mind_map::core::Base64Decode(node.image_png_base64_);
  if (png_bytes.empty()) {
    LOG_WARNING_BUILD("ApplyImageToCanvasNode: base64 decode produced empty PNG for node " << node.id_);
    node.half_extent_override_ = {0.0F, 0.0F};
    return;
  }
  const PngTextureUpload uploaded = UploadPngTexture(png_bytes, "node " + node.id_);
  node.texture_id_ = uploaded.texture_id_;
  if (uploaded.texture_id_ != 0 && uploaded.width_px_ > 0 && uploaded.height_px_ > 0) {
    node.half_extent_override_ =
        mind_map::canvas::NodeHalfExtentForImagePixels(uploaded.width_px_, uploaded.height_px_);
  } else {
    node.half_extent_override_ = {0.0F, 0.0F};
  }
}

void SetNodeImage(MindMapCanvasView& view, size_t idx, std::string_view png_base64) {
  assert(idx < view.nodes_.size());
  if (idx >= view.nodes_.size()) {
    return;
  }
  ApplyImageToCanvasNode(view.nodes_[idx], png_base64);
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
