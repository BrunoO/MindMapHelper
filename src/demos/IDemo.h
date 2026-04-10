#pragma once

#include "imgui.h"

#include <memory>
#include <vector>

namespace mind_map::demos {

struct DemoRenderContext {
  ImDrawList* draw_list = nullptr;
  ImVec2 canvas_p0{};
  ImVec2 canvas_p1{};
  ImVec2 pan_px{};
  float zoom = 1.0F;
  bool canvas_hovered = false;
  ImVec2 mouse_world{};
};

struct DemoPointerState {
  ImVec2 mouse_screen{};
  ImVec2 mouse_world{};
  bool canvas_hovered = false;
};

// Pluggable canvas experiment: shell owns pan/zoom wheel and empty-space pan; demo owns node drag
// and all drawing. Main thread / ImGui frame only.
class IDemo {
 public:
  virtual ~IDemo() = default;

  [[nodiscard]] virtual const char* GetName() const = 0;
  virtual void Reset() {}

  virtual void OnPrimaryDown(const DemoPointerState& ptr) = 0;
  virtual void OnPrimaryDrag(const DemoPointerState& ptr) = 0;
  virtual void OnPrimaryUp() = 0;

  [[nodiscard]] virtual bool IsDraggingContent() const = 0;

  virtual void Render(const DemoRenderContext& ctx) = 0;
};

[[nodiscard]] std::vector<std::unique_ptr<IDemo>> CreateDemoList();

}  // namespace mind_map::demos
