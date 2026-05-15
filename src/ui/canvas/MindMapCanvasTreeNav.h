#pragma once

#include <cstddef>
#include <optional>

namespace mind_map::ui {

class MindMapCanvasView;

namespace canvas {

[[nodiscard]] std::optional<size_t> GetParentOf(const MindMapCanvasView& view, size_t idx);
[[nodiscard]] std::optional<size_t> GetFirstActiveChildOf(const MindMapCanvasView& view, size_t idx);
[[nodiscard]] std::optional<size_t> GetPrevSiblingOf(const MindMapCanvasView& view, size_t idx);
[[nodiscard]] std::optional<size_t> GetNextSiblingOf(const MindMapCanvasView& view, size_t idx);

}  // namespace canvas
}  // namespace mind_map::ui
