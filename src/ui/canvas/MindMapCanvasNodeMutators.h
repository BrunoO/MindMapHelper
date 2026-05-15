#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace mind_map::ui {

class MindMapCanvasView;
struct CanvasNode;

namespace canvas {

void ApplyImageToCanvasNode(CanvasNode& node, std::string_view png_base64);
void SetNodeImage(MindMapCanvasView& view, size_t idx, std::string_view png_base64);
[[nodiscard]] const std::string& GetNodeImageBase64(const MindMapCanvasView& view, size_t idx);
void SetNodeLabel(MindMapCanvasView& view, size_t idx, std::string_view label);
[[nodiscard]] const std::string& GetNodeLabel(const MindMapCanvasView& view, size_t idx);
void SetEdgeLabel(MindMapCanvasView& view, size_t idx, std::string_view label);
[[nodiscard]] const std::string& GetEdgeLabel(const MindMapCanvasView& view, size_t idx);

}  // namespace canvas
}  // namespace mind_map::ui
