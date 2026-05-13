#include "core/imx/ImxImportAdapter.h"
#include "core/MindMapDocument.h"

#include <cassert>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view kDataXml = R"xml(<?xml version="1.0" encoding="UTF-8"?>
<root>
  <floatingIdea id="root-1">
    <property key="com.thinkbuzan.gaia.cell.text" value="Root label"/>
  </floatingIdea>
  <branch source="root-1" target="child-1"/>
  <idea id="child-1">
    <property key="com.thinkbuzan.gaia.cell.text" value="Child label"/>
  </idea>
</root>
)xml";

constexpr std::string_view kMapMetaXml = R"xml(<?xml version="1.0" encoding="UTF-8"?>
<meta>
  <MapMeta title="Test map" author="Unit test" created="2026-05-11"/>
</meta>
)xml";

[[nodiscard]] const mind_map::core::MindMapNode* FindNodeByLabel(
    const mind_map::core::MindMapDocument& doc, std::string_view label) {
  for (const auto& node : doc.nodes_) {
    if (node.label_ == label) {
      return &node;
    }
  }
  return nullptr;
}

[[nodiscard]] const mind_map::core::MindMapNode* FindNodeById(
    const mind_map::core::MindMapDocument& doc, std::string_view node_id) {
  for (const auto& node : doc.nodes_) {
    if (node.id_ == node_id) {
      return &node;
    }
  }
  return nullptr;
}

[[nodiscard]] const mind_map::core::MindMapNodeLayout* FindLayout(
    const mind_map::core::MindMapDocument& doc, std::string_view node_id) {
  for (const auto& layout : doc.layouts_) {
    if (layout.node_id_ == node_id) {
      return &layout;
    }
  }
  return nullptr;
}

void TestConversionNodeCount() {
  const mind_map::core::ImxImportAdapter adapter;
  const auto doc = adapter.ImportFromXml(kDataXml, kMapMetaXml);
  assert(doc.has_value());
  assert(doc->nodes_.size() == 2U);
  assert(doc->edges_.size() == 1U);
  assert(doc->layouts_.size() == 2U);
}

void TestConversionLabels() {
  const mind_map::core::ImxImportAdapter adapter;
  const auto doc = adapter.ImportFromXml(kDataXml, kMapMetaXml);
  assert(doc.has_value());
  assert(FindNodeByLabel(*doc, "Root label")  != nullptr);
  assert(FindNodeByLabel(*doc, "Child label") != nullptr);
}

void TestConversionEdgeStyle() {
  const mind_map::core::ImxImportAdapter adapter;
  const auto doc = adapter.ImportFromXml(kDataXml, kMapMetaXml);
  assert(doc.has_value());
  assert(doc->edges_[0].style_ == "bezier");
}

void TestConversionEdgeConnectivity() {
  const mind_map::core::ImxImportAdapter adapter;
  const auto doc = adapter.ImportFromXml(kDataXml, kMapMetaXml);
  assert(doc.has_value());

  const auto* root  = FindNodeByLabel(*doc, "Root label");
  const auto* child = FindNodeByLabel(*doc, "Child label");
  assert(root  != nullptr);
  assert(child != nullptr);

  assert(doc->edges_[0].parent_id_ == root->id_);
  assert(doc->edges_[0].child_id_  == child->id_);
}

void TestConversionLayout() {
  constexpr float kXSpacing = 260.0F;

  const mind_map::core::ImxImportAdapter adapter;
  const auto doc = adapter.ImportFromXml(kDataXml, kMapMetaXml);
  assert(doc.has_value());

  const auto* root  = FindNodeByLabel(*doc, "Root label");
  const auto* child = FindNodeByLabel(*doc, "Child label");
  assert(root  != nullptr);
  assert(child != nullptr);

  const auto* root_layout  = FindLayout(*doc, root->id_);
  const auto* child_layout = FindLayout(*doc, child->id_);
  assert(root_layout  != nullptr);
  assert(child_layout != nullptr);

  // Root is at depth 0 → x = 0, centred among 1 node → y = 0.
  assert(root_layout->position_.x_ == 0.0F);
  assert(root_layout->position_.y_ == 0.0F);

  // Child is at depth 1 → x = kXSpacing, centred among 1 node → y = 0.
  assert(child_layout->position_.x_ == kXSpacing);
  assert(child_layout->position_.y_ == 0.0F);
}

void TestBranchTextGoesToEdgeLabel() {
  constexpr std::string_view kBranchTextXml = R"xml(<?xml version="1.0" encoding="UTF-8"?>
<root>
  <floatingIdea id="root-1">
    <property key="com.thinkbuzan.gaia.cell.text" value="Root"/>
  </floatingIdea>
  <branch source="root-1" target="child-1">
    <property key="com.thinkbuzan.gaia.cell.text" value="branch caption"/>
  </branch>
  <idea id="child-1"/>
</root>
)xml";

  const mind_map::core::ImxImportAdapter adapter;
  const auto doc = adapter.ImportFromXml(kBranchTextXml, kMapMetaXml);
  assert(doc.has_value());
  assert(doc->edges_.size() == 1U);
  assert(doc->edges_[0].label_ == "branch caption");

  const auto* child = FindNodeById(*doc, doc->edges_[0].child_id_);
  assert(child != nullptr);
  assert(child->label_.empty());

  assert(FindNodeByLabel(*doc, "Root") != nullptr);
}

void TestBranchTextWithNodeBodyText() {
  constexpr std::string_view kBothTextXml = R"xml(<?xml version="1.0" encoding="UTF-8"?>
<root>
  <floatingIdea id="root-1">
    <property key="com.thinkbuzan.gaia.cell.text" value="Root"/>
  </floatingIdea>
  <branch source="root-1" target="child-1">
    <property key="com.thinkbuzan.gaia.cell.text" value="edge caption"/>
  </branch>
  <idea id="child-1">
    <property key="com.thinkbuzan.gaia.cell.text" value="node title"/>
  </idea>
</root>
)xml";

  const mind_map::core::ImxImportAdapter adapter;
  const auto doc = adapter.ImportFromXml(kBothTextXml, kMapMetaXml);
  assert(doc.has_value());
  assert(doc->edges_.size() == 1U);
  assert(doc->edges_[0].label_ == "edge caption");

  const auto* child = FindNodeById(*doc, doc->edges_[0].child_id_);
  assert(child != nullptr);
  assert(child->label_ == "node title");
}

void TestRejectsMissingFloatingIdea() {
  constexpr std::string_view bad_xml = R"xml(<?xml version="1.0" encoding="UTF-8"?>
<root>
  <idea id="orphan-1"/>
</root>
)xml";
  const mind_map::core::ImxImportAdapter adapter;
  assert(!adapter.ImportFromXml(bad_xml, kMapMetaXml).has_value());
}

void TestRejectsMalformedXml() {
  constexpr std::string_view bad_xml = "<<< not xml >>>";
  const mind_map::core::ImxImportAdapter adapter;
  assert(!adapter.ImportFromXml(bad_xml, kMapMetaXml).has_value());
}

}  // namespace

int main() {  // NOLINT(bugprone-exception-escape)
  TestConversionNodeCount();
  TestConversionLabels();
  TestConversionEdgeStyle();
  TestConversionEdgeConnectivity();
  TestConversionLayout();
  TestBranchTextGoesToEdgeLabel();
  TestBranchTextWithNodeBodyText();
  TestRejectsMissingFloatingIdea();
  TestRejectsMalformedXml();
  return 0;
}
