#include "core/mindmap/SampleMindMapTopology.h"

namespace mind_map::core::mindmap {

const std::array<SampleMindMapNodeSpec, kSampleMindMapNodeCount> kSampleMindMapSpecs = {{
    {"Root", -1},
    {"Idea A", 0},
    {"Idea B", 0},
    {"Idea C", 0},
    {"Detail A1", 1},
    {"Detail A2", 1},
    {"Detail B1", 2},
}};

}  // namespace mind_map::core::mindmap
