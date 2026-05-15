#pragma once

namespace mind_map::app { class DocumentSessionService; }
namespace mind_map::ui::commands { class CommandHistory; }
namespace mind_map::ui { class MindMapCanvasView; struct MindMapCanvasRenderContext; }

namespace mind_map::ui {

// Renders the inline text-editing widget over the node that is currently in
// BeginEditing state.  Must be called after MindMapCanvasView::Render() so the
// widget composites on top of the draw-list content.
void RenderNodeEditOverlay(const MindMapCanvasRenderContext& ctx,
                           MindMapCanvasView& canvas,
                           mind_map::app::DocumentSessionService& session,
                           commands::CommandHistory& history);

}  // namespace mind_map::ui
