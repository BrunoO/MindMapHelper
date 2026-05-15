#include "core/JsonNativeDocumentRepository.h"

#include "utils/Logger.h"

#include <nlohmann/json.hpp>

#include <fstream>

namespace mind_map::core {

namespace {

constexpr std::string_view kFormatName = "mindmaphelper";
constexpr int kCurrentVersion = 1;
constexpr int kJsonIndent = 2;

MindMapDocument FromJson(const nlohmann::json& j) {
  MindMapDocument doc;

  for (const auto& n : j.at("nodes")) {
    MindMapNode node;
    node.id_ = n.at("id").get<std::string>();
    node.label_ = n.at("label").get<std::string>();
    node.image_png_base64_ = n.value("image", std::string{});
    node.collapsed_ = n.value("collapsed", false);
    doc.nodes_.push_back(std::move(node));
  }

  for (const auto& e : j.at("edges")) {
    MindMapEdge edge;
    edge.id_ = e.at("id").get<std::string>();
    edge.parent_id_ = e.at("parent").get<std::string>();
    edge.child_id_ = e.at("child").get<std::string>();
    edge.style_ = e.at("style").get<std::string>();
    edge.label_ = e.value("label", std::string{});
    doc.edges_.push_back(std::move(edge));
  }

  for (const auto& l : j.at("layout")) {
    MindMapNodeLayout layout;
    layout.node_id_ = l.at("node").get<std::string>();
    layout.position_.x_ = l.at("x").get<float>();
    layout.position_.y_ = l.at("y").get<float>();
    layout.size_w_ = l.value("hw", 0.0F);
    layout.size_h_ = l.value("hh", 0.0F);
    doc.layouts_.push_back(std::move(layout));
  }

  const auto& vp = j.at("viewport");
  doc.viewport_.pan_.x_ = vp.at("pan_x").get<float>();
  doc.viewport_.pan_.y_ = vp.at("pan_y").get<float>();
  doc.viewport_.zoom_ = vp.at("zoom").get<float>();

  return doc;
}

nlohmann::json ToJson(const MindMapDocument& doc) {
  nlohmann::json j;
  j["format"] = kFormatName;
  j["version"] = kCurrentVersion;

  j["nodes"] = nlohmann::json::array();
  for (const auto& node : doc.nodes_) {
    nlohmann::json n = {{"id", node.id_}, {"label", node.label_}};
    if (!node.image_png_base64_.empty()) {
      n["image"] = node.image_png_base64_;
    }
    if (node.collapsed_) { n["collapsed"] = true; }
    j["nodes"].push_back(std::move(n));
  }

  j["edges"] = nlohmann::json::array();
  for (const auto& edge : doc.edges_) {
    nlohmann::json e_json = {
      {"id", edge.id_},
      {"parent", edge.parent_id_},
      {"child", edge.child_id_},
      {"style", edge.style_},
    };
    if (!edge.label_.empty()) { e_json["label"] = edge.label_; }
    j["edges"].push_back(std::move(e_json));
  }

  j["layout"] = nlohmann::json::array();
  for (const auto& layout : doc.layouts_) {
    nlohmann::json l_json = {
      {"node", layout.node_id_},
      {"x", layout.position_.x_},
      {"y", layout.position_.y_},
    };
    if (layout.size_w_ > 0.0F) { l_json["hw"] = layout.size_w_; }
    if (layout.size_h_ > 0.0F) { l_json["hh"] = layout.size_h_; }
    j["layout"].push_back(std::move(l_json));
  }

  j["viewport"] = {
    {"pan_x", doc.viewport_.pan_.x_},
    {"pan_y", doc.viewport_.pan_.y_},
    {"zoom", doc.viewport_.zoom_},
  };

  return j;
}

}  // namespace

std::optional<MindMapDocument> JsonNativeDocumentRepository::Load(std::string_view path) const {
  const std::string path_str{path};
  std::ifstream file(path_str);
  if (!file.is_open()) {
    LOG_ERROR_BUILD("JsonNativeDocumentRepository: cannot open '" << path << '\'');
    return std::nullopt;
  }

  try {
    const nlohmann::json j = nlohmann::json::parse(file);

    if (j.at("format").get<std::string>() != kFormatName) {
      LOG_ERROR("JsonNativeDocumentRepository: not a mindmaphelper file");
      return std::nullopt;
    }

    if (const int version = j.at("version").get<int>(); version != kCurrentVersion) {
      LOG_ERROR_BUILD("JsonNativeDocumentRepository: unsupported version " << version);
      return std::nullopt;
    }

    return FromJson(j);
  }
  catch (const nlohmann::json::exception& e) {
    (void)e;
    LOG_ERROR_BUILD("JsonNativeDocumentRepository: load failed — " << e.what());
    return std::nullopt;
  }
}

bool JsonNativeDocumentRepository::Save(std::string_view path, const MindMapDocument& doc) const {
  const std::string path_str{path};
  std::ofstream file(path_str);
  if (!file.is_open()) {
    LOG_ERROR_BUILD("JsonNativeDocumentRepository: cannot write '" << path << '\'');
    return false;
  }

  try {
    file << ToJson(doc).dump(kJsonIndent);
    if (!file.good()) {
      LOG_ERROR_BUILD("JsonNativeDocumentRepository: write error for '" << path << '\'');
      return false;
    }
    return true;
  }
  catch (const nlohmann::json::exception& e) {
    (void)e;
    LOG_ERROR_BUILD("JsonNativeDocumentRepository: save failed — " << e.what());
    return false;
  }
}

}  // namespace mind_map::core
