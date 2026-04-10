#include "demos/IDemo.h"

#include "demos/DemoBezierMindMap.h"
#include "demos/DemoStraightMindMap.h"

#include <memory>
#include <utility>
#include <vector>

namespace mind_map::demos {

std::vector<std::unique_ptr<IDemo>> CreateDemoList() {
  std::vector<std::unique_ptr<IDemo>> demos;
  demos.push_back(std::make_unique<DemoBezierMindMap>());
  demos.push_back(std::make_unique<DemoStraightMindMap>());
  return demos;
}

}  // namespace mind_map::demos
