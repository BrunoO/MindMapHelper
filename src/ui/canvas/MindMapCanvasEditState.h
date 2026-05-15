#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace mind_map::ui {
class MindMapCanvasView;
}

namespace mind_map::ui::canvas {

// Free-function accessors for the inline-editing state of MindMapCanvasView.
// These are friends of the class so the class method count stays under Sonar's limit.
[[nodiscard]] bool IsEditing(const MindMapCanvasView& view);
[[nodiscard]] std::optional<size_t> GetEditingNode(const MindMapCanvasView& view);
[[nodiscard]] std::string& GetEditBuffer(MindMapCanvasView& view);

}  // namespace mind_map::ui::canvas
