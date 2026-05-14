#pragma once

#include "core/MindMapDocument.h"

namespace mind_map::app {

/// Builds a read-only guide map matching current product features (canvas, branches, menus, shortcuts).
[[nodiscard]] mind_map::core::MindMapDocument BuildHelpMindMapDocument();

}  // namespace mind_map::app
