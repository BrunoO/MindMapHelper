#include "core/JsonNativeDocumentRepository.h"
#include "core/MindMapDocument.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

constexpr float kChildPosX  = 260.0F;
constexpr float kChildPosY  = -140.0F;
constexpr float kPanX       = 40.0F;
constexpr float kPanY       = 120.0F;
constexpr float kZoom       = 1.5F;

[[nodiscard]] mind_map::core::MindMapDocument MakeTestDocument() {
  mind_map::core::MindMapDocument doc;

  mind_map::core::MindMapNode root;
  root.id_    = "node-1";
  root.label_ = "Root";
  doc.nodes_.push_back(std::move(root));

  mind_map::core::MindMapNode child;
  child.id_    = "node-2";
  child.label_ = "Child";
  doc.nodes_.push_back(std::move(child));

  mind_map::core::MindMapEdge edge;
  edge.id_        = "edge-1";
  edge.parent_id_ = "node-1";
  edge.child_id_  = "node-2";
  edge.style_     = "bezier";
  doc.edges_.push_back(std::move(edge));

  mind_map::core::MindMapNodeLayout root_layout;
  root_layout.node_id_      = "node-1";
  root_layout.position_.x_  = 0.0F;
  root_layout.position_.y_  = 0.0F;
  doc.layouts_.push_back(std::move(root_layout));

  mind_map::core::MindMapNodeLayout child_layout;
  child_layout.node_id_      = "node-2";
  child_layout.position_.x_  = kChildPosX;
  child_layout.position_.y_  = kChildPosY;
  doc.layouts_.push_back(std::move(child_layout));

  doc.viewport_.pan_.x_ = kPanX;
  doc.viewport_.pan_.y_ = kPanY;
  doc.viewport_.zoom_   = kZoom;

  return doc;
}

void TestRoundTrip() {
  const std::filesystem::path tmp =
      std::filesystem::temp_directory_path() / "mindmap_helper_json_round_trip_test.mmh";

  const mind_map::core::MindMapDocument saved = MakeTestDocument();
  const mind_map::core::JsonNativeDocumentRepository repo;

  assert(repo.Save(tmp.string(), saved));

  const auto loaded_opt = repo.Load(tmp.string());
  assert(loaded_opt.has_value());
  const mind_map::core::MindMapDocument& loaded = *loaded_opt;

  assert(loaded.nodes_.size() == 2U);
  assert(loaded.nodes_[0].id_    == "node-1");
  assert(loaded.nodes_[0].label_ == "Root");
  assert(loaded.nodes_[1].id_    == "node-2");
  assert(loaded.nodes_[1].label_ == "Child");

  assert(loaded.edges_.size() == 1U);
  assert(loaded.edges_[0].id_        == "edge-1");
  assert(loaded.edges_[0].parent_id_ == "node-1");
  assert(loaded.edges_[0].child_id_  == "node-2");
  assert(loaded.edges_[0].style_     == "bezier");

  assert(loaded.layouts_.size() == 2U);

  assert(loaded.viewport_.pan_.x_ == kPanX);
  assert(loaded.viewport_.pan_.y_ == kPanY);
  assert(loaded.viewport_.zoom_   == kZoom);

  static_cast<void>(std::filesystem::remove(tmp));
}

void WriteFile(const std::filesystem::path& path, const std::string& content) {
  std::ofstream out(path.string());
  assert(out.is_open());
  out << content;
}

void TestRejectsWrongFormat() {
  const std::filesystem::path tmp =
      std::filesystem::temp_directory_path() / "mindmap_helper_json_wrong_format_test.mmh";
  WriteFile(tmp, R"({"format":"other","version":1,"nodes":[],"edges":[],"layout":[],"viewport":{"pan_x":0,"pan_y":0,"zoom":1}})");
  const mind_map::core::JsonNativeDocumentRepository repo;
  assert(!repo.Load(tmp.string()).has_value());
  static_cast<void>(std::filesystem::remove(tmp));
}

void TestRejectsWrongVersion() {
  const std::filesystem::path tmp =
      std::filesystem::temp_directory_path() / "mindmap_helper_json_wrong_version_test.mmh";
  WriteFile(tmp, R"({"format":"mindmaphelper","version":99,"nodes":[],"edges":[],"layout":[],"viewport":{"pan_x":0,"pan_y":0,"zoom":1}})");
  const mind_map::core::JsonNativeDocumentRepository repo;
  assert(!repo.Load(tmp.string()).has_value());
  static_cast<void>(std::filesystem::remove(tmp));
}

void TestRejectsMalformedJson() {
  const std::filesystem::path tmp =
      std::filesystem::temp_directory_path() / "mindmap_helper_json_malformed_test.mmh";
  WriteFile(tmp, "this is not json at all {{{");
  const mind_map::core::JsonNativeDocumentRepository repo;
  assert(!repo.Load(tmp.string()).has_value());
  static_cast<void>(std::filesystem::remove(tmp));
}

void TestRejectsMissingFile() {
  const mind_map::core::JsonNativeDocumentRepository repo;
  assert(!repo.Load("/tmp/mindmap_helper_definitely_does_not_exist_xyzzy.mmh").has_value());
}

}  // namespace

int main() {  // NOLINT(bugprone-exception-escape)
  TestRoundTrip();
  TestRejectsWrongFormat();
  TestRejectsWrongVersion();
  TestRejectsMalformedJson();
  TestRejectsMissingFile();
  return 0;
}
