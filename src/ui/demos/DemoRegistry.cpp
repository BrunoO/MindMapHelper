#include "ui/demos/IDemo.h"

#include "ui/demos/DemoBezierMindMap.h"
#include "ui/demos/DemoStraightMindMap.h"
#include "ui/demos/DemoTaperOrganicMindMap.h"

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
