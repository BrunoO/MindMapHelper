#include "demos/IDemo.h"

#include "demos/DemoBezierMindMap.h"
#include "demos/DemoStraightMindMap.h"
#include "demos/DemoTaperOrganicMindMap.h"

#include <memory>
#include <vector>

namespace mind_map::demos {

std::vector<std::unique_ptr<IDemo>> CreateDemoList() {
  std::vector<std::unique_ptr<IDemo>> demos;
  demos.push_back(std::make_unique<DemoBezierMindMap>());
  demos.push_back(std::make_unique<DemoStraightMindMap>());
  demos.push_back(std::make_unique<DemoTaperOrganicMindMap>());
  return demos;
}

}  // namespace mind_map::demos
