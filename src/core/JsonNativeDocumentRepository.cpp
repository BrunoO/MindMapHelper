#include "core/JsonNativeDocumentRepository.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>

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
    doc.nodes_.push_back(std::move(node));
  }

  for (const auto& e : j.at("edges")) {
    MindMapEdge edge;
    edge.id_ = e.at("id").get<std::string>();
    edge.parent_id_ = e.at("parent").get<std::string>();
    edge.child_id_ = e.at("child").get<std::string>();
    edge.style_ = e.at("style").get<std::string>();
    doc.edges_.push_back(std::move(edge));
  }

  for (const auto& l : j.at("layout")) {
    MindMapNodeLayout layout;
    layout.node_id_ = l.at("node").get<std::string>();
    layout.position_.x_ = l.at("x").get<float>();
    layout.position_.y_ = l.at("y").get<float>();
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
    j["nodes"].push_back({{"id", node.id_}, {"label", node.label_}});
  }

  j["edges"] = nlohmann::json::array();
  for (const auto& edge : doc.edges_) {
    j["edges"].push_back({
      {"id", edge.id_},
      {"parent", edge.parent_id_},
      {"child", edge.child_id_},
      {"style", edge.style_},
    });
  }

  j["layout"] = nlohmann::json::array();
  for (const auto& layout : doc.layouts_) {
    j["layout"].push_back({
      {"node", layout.node_id_},
      {"x", layout.position_.x_},
      {"y", layout.position_.y_},
    });
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
    std::cerr << "JsonNativeDocumentRepository: cannot open '" << path << "'\n";
    return std::nullopt;
  }

  try {
    const nlohmann::json j = nlohmann::json::parse(file);

    if (j.at("format").get<std::string>() != kFormatName) {
      std::cerr << "JsonNativeDocumentRepository: not a mindmaphelper file\n";
      return std::nullopt;
    }

    if (const int version = j.at("version").get<int>(); version != kCurrentVersion) {
      std::cerr << "JsonNativeDocumentRepository: unsupported version " << version << '\n';
      return std::nullopt;
    }

    return FromJson(j);
  }
  catch (const std::exception& e) {
    std::cerr << "JsonNativeDocumentRepository: load failed — " << e.what() << '\n';
    return std::nullopt;
  }
}

bool JsonNativeDocumentRepository::Save(std::string_view path, const MindMapDocument& doc) const {
  const std::string path_str{path};
  std::ofstream file(path_str);
  if (!file.is_open()) {
    std::cerr << "JsonNativeDocumentRepository: cannot write '" << path << "'\n";
    return false;
  }

  try {
    file << ToJson(doc).dump(kJsonIndent);
    if (!file.good()) {
      std::cerr << "JsonNativeDocumentRepository: write error for '" << path << "'\n";
      return false;
    }
    return true;
  }
  catch (const std::exception& e) {
    std::cerr << "JsonNativeDocumentRepository: save failed — " << e.what() << '\n';
    return false;
  }
}

}  // namespace mind_map::core
