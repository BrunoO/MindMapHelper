#include "app/HelpMindMapDocument.h"

#include "core/mindmap/UuidGenerator.h"

#include <string>
#include <string_view>
#include <utility>

namespace mind_map::app {
namespace {

// Default help viewport pan matches mind_map::ui::kInitialPanX / kInitialPanY so opening the guide feels
// like a normal session.
constexpr float kHelpViewportPanX = 40.0F;
constexpr float kHelpViewportPanY = 120.0F;

constexpr float kHelpRootX       = -1080.0F;
constexpr float kHelpRootY       = -40.0F;
constexpr float kHelpHubX        = -700.0F;
constexpr float kHelpLeafX       = 320.0F;
constexpr float kHelpLeafDy      = 58.0F;
constexpr float kHelpCanvasHubY  = -518.0F;
constexpr float kHelpFirstLeafY  = kHelpCanvasHubY - 80.0F;
constexpr float kHelpBranchesHubY = -248.0F;
constexpr float kHelpFileHubY    = 22.0F;
constexpr float kHelpEditHubY    = 292.0F;
constexpr float kHelpCliHubY     = 562.0F;
constexpr float kHelpViewportZoom = 1.0F;

class HelpDocBuilder {
 public:
  [[nodiscard]] std::string AddNode(std::string label, mind_map::core::Vec2 pos) {
    const std::string id = mind_map::core::mindmap::GenerateUuidV4();
    mind_map::core::MindMapNode node;
    node.id_ = id;
    node.label_ = std::move(label);
    doc_.nodes_.push_back(std::move(node));

    mind_map::core::MindMapNodeLayout layout;
    layout.node_id_ = id;
    layout.position_ = pos;
    doc_.layouts_.push_back(std::move(layout));
    return id;
  }

  void AddEdge(std::string_view parent_id, std::string_view child_id) {
    mind_map::core::MindMapEdge edge;
    edge.id_ = mind_map::core::mindmap::GenerateUuidV4();
    edge.parent_id_ = parent_id;
    edge.child_id_ = child_id;
    edge.style_ = "bezier";
    doc_.edges_.push_back(std::move(edge));
  }

  [[nodiscard]] std::string AddChild(const std::string& parent_id, std::string label,
                                      mind_map::core::Vec2 pos) {
    const std::string child_id = AddNode(std::move(label), pos);
    AddEdge(parent_id, child_id);
    return child_id;
  }

  void AddLeaf(const std::string& parent_id, std::string label, mind_map::core::Vec2 pos) {
    static_cast<void>(AddChild(parent_id, std::move(label), pos));
  }

  [[nodiscard]] mind_map::core::MindMapDocument Release() { return std::move(doc_); }

 private:
  mind_map::core::MindMapDocument doc_;
};

}  // namespace

mind_map::core::MindMapDocument BuildHelpMindMapDocument() {
  // Layout: root (left) → topic hubs (mid) → detail leaves (right). Each leaf gets its own world Y so
  // long labels do not stack on the same point (previously every leaf used the same X, which collided).
  HelpDocBuilder b;
  const std::string root = b.AddNode("MindMap Helper — guide", {kHelpRootX, kHelpRootY});

  const std::string canvas = b.AddChild(root, "Canvas & selection", {kHelpHubX, kHelpCanvasHubY});
  float leaf_y = kHelpFirstLeafY;
  b.AddLeaf(canvas, "Pan: drag empty canvas (not a node).", {kHelpLeafX, leaf_y});
  leaf_y += kHelpLeafDy;
  b.AddLeaf(canvas, "Zoom: mouse wheel while the canvas is hovered.", {kHelpLeafX, leaf_y});
  leaf_y += kHelpLeafDy;
  b.AddLeaf(canvas, "Move nodes: drag the node body.", {kHelpLeafX, leaf_y});
  leaf_y += kHelpLeafDy;
  b.AddLeaf(canvas, "Click a node to select; click empty space or root to clear incoming-edge style target.",
              {kHelpLeafX, leaf_y});
  leaf_y += kHelpLeafDy;

  const std::string branches = b.AddChild(root, "Branches & styles", {kHelpHubX, kHelpBranchesHubY});
  b.AddLeaf(branches, "Select a non-root node → toolbar combo edits that incoming edge.", {kHelpLeafX, leaf_y});
  leaf_y += kHelpLeafDy;
  b.AddLeaf(branches, "\"Set all branches to\" applies one style to every child edge at once.", {kHelpLeafX, leaf_y});
  leaf_y += kHelpLeafDy;
  b.AddLeaf(branches, "Styles: Bezier, Orthogonal, Organic taper (stored in .mmh).", {kHelpLeafX, leaf_y});
  leaf_y += kHelpLeafDy;
  b.AddLeaf(branches, "Optional edge label text is saved with the document.", {kHelpLeafX, leaf_y});
  leaf_y += kHelpLeafDy;

  const std::string file_menu = b.AddChild(root, "File & windows", {kHelpHubX, kHelpFileHubY});
  b.AddLeaf(file_menu, "New, Open (.mmh / .imx), Save, Save As (.mmh), Import (.imx).", {kHelpLeafX, leaf_y});
  leaf_y += kHelpLeafDy;
  b.AddLeaf(file_menu, "New Window (blank map); Open in New Window… (pick a file).", {kHelpLeafX, leaf_y});
  leaf_y += kHelpLeafDy;
  b.AddLeaf(file_menu, "Reset layout: restore saved initial positions. Reset view: default pan/zoom.", {kHelpLeafX, leaf_y});
  leaf_y += kHelpLeafDy;

  const std::string edit_view = b.AddChild(root, "Edit, view & clipboard", {kHelpHubX, kHelpEditHubY});
  b.AddLeaf(edit_view, "Undo / Redo; Insert child (Tab or Insert); Delete node (Delete, not root).", {kHelpLeafX, leaf_y});
  leaf_y += kHelpLeafDy;
  b.AddLeaf(edit_view, "Collapse / expand subtree: ▶/▼ on node or Space when a parent is selected.", {kHelpLeafX, leaf_y});
  leaf_y += kHelpLeafDy;
  b.AddLeaf(edit_view, "Paste: Cmd/Ctrl+V with a node selected (image or text from clipboard).", {kHelpLeafX, leaf_y});
  leaf_y += kHelpLeafDy;
  b.AddLeaf(edit_view, "View menu: zoom in/out, reset view, toggle status bar.", {kHelpLeafX, leaf_y});
  leaf_y += kHelpLeafDy;
  b.AddLeaf(edit_view, "Shortcuts: Cmd/Ctrl+= / -= zoom; Cmd/Ctrl+0 reset view.", {kHelpLeafX, leaf_y});
  leaf_y += kHelpLeafDy;

  const std::string cli = b.AddChild(root, "Command line", {kHelpHubX, kHelpCliHubY});
  b.AddLeaf(cli, "First argument: path to a .mmh or .imx file opens it on launch (ignored if --help is used).",
              {kHelpLeafX, leaf_y});
  leaf_y += kHelpLeafDy;
  b.AddLeaf(cli, "MindMapHelper --help opens this guide as the startup map.", {kHelpLeafX, leaf_y});

  mind_map::core::MindMapDocument doc = b.Release();
  doc.viewport_.pan_ = {kHelpViewportPanX, kHelpViewportPanY};
  doc.viewport_.zoom_ = kHelpViewportZoom;
  return doc;
}

}  // namespace mind_map::app
