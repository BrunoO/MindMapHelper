#pragma once

#include "ui/demos/IDemo.h"
#include "ui/demos/SampleMindMapGraph.h"

#include <array>

namespace mind_map::demos {

class DemoStraightMindMap final : public IDemo {
 public:
  [[nodiscard]] const char* GetName() const override;
  void Reset() override;
  void OnPrimaryDown(const DemoPointerState& ptr) override;
  void OnPrimaryDrag(const DemoPointerState& ptr) override;
  void OnPrimaryUp() override;
  [[nodiscard]] bool IsDraggingContent() const override;
  void Render(const DemoRenderContext& ctx) override;

 private:
  std::array<ImVec2, kSampleMindMapNodeCount> pos_world_ = InitialSampleMapPositions();
  int dragging_node_ = -1;
  ImVec2 grab_offset_world_ = {0.0F, 0.0F};
};

}  // namespace mind_map::demos
