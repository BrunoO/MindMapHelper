#include "core/imx/ImxMindMapLoader.h"

#include <pugixml.hpp>
#include <zip.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mind_map::core {

namespace {

constexpr std::string_view kGaiaCellTextKey = "com.thinkbuzan.gaia.cell.text";
constexpr std::string_view kGaiaHTMLLabelKey = "com.thinkbuzan.gaia.entities.HTMLLabel";
constexpr std::string_view kImageStyleKey = "image=";

[[nodiscard]] std::string AttrOrEmpty(const pugi::xml_node& node, const char* key) {
  if (const pugi::xml_attribute attr = node.attribute(key)) {
    return {attr.value()};
  }
  return {};
}

[[nodiscard]] pugi::xml_node FindFirstElementByName(const pugi::xml_node& root, const char* name) {
  std::vector<pugi::xml_node> stack;
  if (!root.empty()) {
    stack.push_back(root);
  }
  while (!stack.empty()) {
    const pugi::xml_node node = stack.back();
    stack.pop_back();
    if (node.type() == pugi::node_element && std::strcmp(node.name(), name) == 0) {
      return node;
    }
    for (pugi::xml_node child = node.last_child(); !child.empty(); child = child.previous_sibling()) {
      stack.push_back(child);
    }
  }
  return {};
}

[[nodiscard]] std::string StripHtmlTags(std::string_view html) {
  std::string result;
  result.reserve(html.size());
  bool in_tag = false;
  for (const char ch : html) {
    if (ch == '<') {
      in_tag = true;
    } else if (ch == '>') {
      in_tag = false;
    } else if (!in_tag) {
      result += ch;
    }
  }
  const auto first = result.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  return result.substr(first, result.find_last_not_of(" \t\r\n") - first + 1);
}

[[nodiscard]] std::optional<std::string> ExtractHtmlLabelText(const pugi::xml_node& property_node) {
  if (const pugi::xml_attribute key_attr = property_node.attribute("key");
      key_attr.empty() || std::string_view{key_attr.value()} != kGaiaHTMLLabelKey) {
    return std::nullopt;
  }
  const pugi::xml_node html_label = property_node.child("HTMLLabel");
  if (html_label.empty()) {
    return std::nullopt;
  }
  std::string inner;
  std::vector<pugi::xml_node> stack;
  stack.push_back(html_label);
  while (!stack.empty()) {
    const pugi::xml_node n = stack.back();
    stack.pop_back();
    if (n.type() == pugi::node_pcdata || n.type() == pugi::node_cdata) {
      inner += n.value();
    }
    for (pugi::xml_node child = n.last_child(); !child.empty(); child = child.previous_sibling()) {
      stack.push_back(child);
    }
  }
  if (inner.empty()) {
    return std::nullopt;
  }
  return StripHtmlTags(inner);
}

void MaybeCaptureHtmlLabelFallback(const pugi::xml_node& property_node, std::string* html_label_fallback) {
  assert(html_label_fallback != nullptr);
  if (!html_label_fallback->empty()) {
    return;
  }
  if (auto result = ExtractHtmlLabelText(property_node)) {
    *html_label_fallback = std::move(*result);
  }
}

[[nodiscard]] std::optional<std::string> ExtractCellTextFromPropertyNode(const pugi::xml_node& node) {
  if (const pugi::xml_attribute key_attr = node.attribute("key");
      key_attr.empty() || std::string_view{key_attr.value()} != kGaiaCellTextKey) {
    return std::nullopt;
  }
  if (const pugi::xml_attribute value_attr = node.attribute("value")) {
    return std::string{value_attr.value()};
  }
  return std::string{node.text().get()};
}

[[nodiscard]] std::string ExtractCellText(const pugi::xml_node& node_with_id) {
  std::string html_label_fallback;
  std::vector<pugi::xml_node> stack;
  stack.push_back(node_with_id);
  while (!stack.empty()) {
    const pugi::xml_node node = stack.back();
    stack.pop_back();
    if (node.type() == pugi::node_element && std::strcmp(node.name(), "property") == 0) {
      if (auto result = ExtractCellTextFromPropertyNode(node)) {
        return *result;
      }
      MaybeCaptureHtmlLabelFallback(node, &html_label_fallback);
    }
    for (pugi::xml_node child = node.last_child(); !child.empty(); child = child.previous_sibling()) {
      stack.push_back(child);
    }
  }
  return html_label_fallback;
}

[[nodiscard]] std::string ExtractImageIdFromStyle(std::string_view style) {
  const auto pos = style.find(kImageStyleKey);
  if (pos == std::string_view::npos) {
    return {};
  }
  const std::string_view value = style.substr(pos + kImageStyleKey.size());
  const auto end = value.find_first_of(";, \t");
  return std::string(end == std::string_view::npos ? value : value.substr(0, end));
}

[[nodiscard]] std::unordered_map<std::string, std::string> BuildIdToImageAssetIdMap(
    const pugi::xml_document& doc) {
  std::unordered_map<std::string, std::string> result;
  std::vector<pugi::xml_node> stack;
  if (const pugi::xml_node root = doc.document_element()) {
    stack.push_back(root);
  }
  while (!stack.empty()) {
    const pugi::xml_node node = stack.back();
    stack.pop_back();
    if (node.type() == pugi::node_element) {
      const pugi::xml_attribute id_attr = node.attribute("id");
      const pugi::xml_attribute style_attr = node.attribute("style");
      if (!id_attr.empty() && !style_attr.empty()) {
        const std::string image_id = ExtractImageIdFromStyle(style_attr.value());
        if (!image_id.empty()) {
          result.try_emplace(std::string{id_attr.value()}, image_id);
        }
      }
    }
    for (pugi::xml_node child = node.last_child(); !child.empty(); child = child.previous_sibling()) {
      stack.push_back(child);
    }
  }
  return result;
}

void UpdateIdTextMap(std::unordered_map<std::string, std::string>& map, const pugi::xml_node& node) {
  const pugi::xml_attribute id_attr = node.attribute("id");
  if (id_attr.empty()) {
    return;
  }
  const std::string id{id_attr.value()};
  if (id.empty()) {
    return;
  }
  const std::string text = ExtractCellText(node);
  auto [it, inserted] = map.try_emplace(id, text);
  if (!inserted && !text.empty()) {
    it->second = text;
  }
}

[[nodiscard]] std::unordered_map<std::string, std::string> BuildIdToTextMap(const pugi::xml_document& doc) {
  std::unordered_map<std::string, std::string> id_to_text;
  std::vector<pugi::xml_node> stack;
  if (const pugi::xml_node root = doc.document_element()) {
    stack.push_back(root);
  }
  while (!stack.empty()) {
    const pugi::xml_node node = stack.back();
    stack.pop_back();
    if (node.type() == pugi::node_element) {
      UpdateIdTextMap(id_to_text, node);
    }
    for (pugi::xml_node child = node.last_child(); !child.empty(); child = child.previous_sibling()) {
      stack.push_back(child);
    }
  }
  return id_to_text;
}

struct ImxBranchEdge {
  std::string source_;
  std::string target_;
  std::string text_;
};

[[nodiscard]] std::vector<ImxBranchEdge> CollectBranches(const pugi::xml_document& doc) {
  std::vector<ImxBranchEdge> edges;
  std::vector<pugi::xml_node> stack;
  if (const pugi::xml_node root = doc.document_element()) {
    stack.push_back(root);
  }
  while (!stack.empty()) {
    const pugi::xml_node node = stack.back();
    stack.pop_back();
    if (node.type() == pugi::node_element && std::strcmp(node.name(), "branch") == 0) {
      const std::string source = AttrOrEmpty(node, "source");
      const std::string target = AttrOrEmpty(node, "target");
      if (!source.empty() && !target.empty()) {
        edges.push_back({source, target, ExtractCellText(node)});
      }
    }
    for (pugi::xml_node child = node.last_child(); !child.empty(); child = child.previous_sibling()) {
      stack.push_back(child);
    }
  }
  return edges;
}

[[nodiscard]] std::unordered_map<std::string, std::vector<std::string>> BuildChildrenMap(
    const std::vector<ImxBranchEdge>& edges) {
  std::unordered_map<std::string, std::vector<std::string>> children;
  for (const auto& edge : edges) {
    std::vector<std::string>& bucket = children[edge.source_];
    if (std::find(bucket.begin(), bucket.end(), edge.target_) == bucket.end()) {  // NOLINT(llvm-use-ranges)
      bucket.push_back(edge.target_);
    }
  }
  return children;
}

void ApplyImageAssetId(ImxNode& node,
                       const std::unordered_map<std::string, std::string>& id_to_image_asset) {
  if (const auto it = id_to_image_asset.find(node.id_); it != id_to_image_asset.end()) {
    node.image_asset_id_ = it->second;
  }
}

[[nodiscard]] std::optional<ImxMindMapModel> ParseDataXml(std::string_view data_xml) {
  pugi::xml_document doc;
  if (const pugi::xml_parse_result parse_result =
          doc.load_buffer(data_xml.data(), data_xml.size(), pugi::parse_default, pugi::encoding_utf8);
      parse_result.status != pugi::status_ok) {
    return std::nullopt;
  }

  const pugi::xml_node floating = FindFirstElementByName(doc.document_element(), "floatingIdea");
  if (!floating) {
    return std::nullopt;
  }
  const std::string root_id = AttrOrEmpty(floating, "id");
  if (root_id.empty()) {
    return std::nullopt;
  }

  const std::unordered_map<std::string, std::string> element_text = BuildIdToTextMap(doc);
  const std::unordered_map<std::string, std::string> id_to_image_asset = BuildIdToImageAssetIdMap(doc);
  const std::vector<ImxBranchEdge> edges = CollectBranches(doc);
  const std::unordered_map<std::string, std::vector<std::string>> children_by_parent = BuildChildrenMap(edges);

  // In real IMX files the node label lives on the incoming branch edge, not on the
  // branchNode target element. Build a target→text map from branch edge texts.
  std::unordered_map<std::string, std::string> branch_text_by_target;
  for (const auto& edge : edges) {
    if (!edge.text_.empty()) {
      branch_text_by_target.try_emplace(edge.target_, edge.text_);
    }
  }

  // Only include structural node IDs (graph endpoints), never branch element IDs.
  std::unordered_set<std::string> all_ids;
  all_ids.insert(root_id);
  for (const auto& edge : edges) {
    all_ids.insert(edge.source_);
    all_ids.insert(edge.target_);
  }

  // Branch edge text takes priority; fall back to the element's own text.
  auto resolve_text = [&](const std::string& id) -> std::string {
    if (const auto it = branch_text_by_target.find(id); it != branch_text_by_target.end()) {
      return it->second;
    }
    if (const auto it = element_text.find(id); it != element_text.end()) {
      return it->second;
    }
    return {};
  };

  std::vector<ImxNode> ordered_nodes;
  std::unordered_set<std::string> visited;
  std::deque<std::string> queue;
  queue.push_back(root_id);

  while (!queue.empty()) {
    const std::string id = std::move(queue.front());
    queue.pop_front();
    if (!visited.insert(id).second) {
      continue;
    }
    ImxNode node;
    node.id_ = id;
    node.text_ = resolve_text(id);
    ApplyImageAssetId(node, id_to_image_asset);
    if (const auto it = children_by_parent.find(id); it != children_by_parent.end()) {
      node.children_ = it->second;
      for (const std::string& child_id : node.children_) {
        queue.push_back(child_id);
      }
    }
    ordered_nodes.push_back(std::move(node));
  }

  for (const std::string& id : all_ids) {
    if (visited.find(id) != visited.end()) {
      continue;
    }
    ImxNode node;
    node.id_ = id;
    node.text_ = resolve_text(id);
    ApplyImageAssetId(node, id_to_image_asset);
    if (const auto it = children_by_parent.find(id); it != children_by_parent.end()) {
      node.children_ = it->second;
    }
    ordered_nodes.push_back(std::move(node));
  }

  ImxMindMapModel model;
  model.root_id_ = root_id;
  model.nodes_ = std::move(ordered_nodes);
  return {std::move(model)};
}

[[nodiscard]] ImxMapMeta ParseMapMetaXml(std::string_view mapmeta_xml) {
  ImxMapMeta meta;
  if (mapmeta_xml.empty()) {
    return meta;
  }
  pugi::xml_document doc;
  if (const pugi::xml_parse_result parse_result =
          doc.load_buffer(mapmeta_xml.data(), mapmeta_xml.size(), pugi::parse_default, pugi::encoding_utf8);
      parse_result.status != pugi::status_ok) {
    return meta;
  }
  const pugi::xml_node map_meta = FindFirstElementByName(doc.document_element(), "MapMeta");
  if (!map_meta) {
    return meta;
  }
  // Try attribute on MapMeta (unit-test format), then child element (real IMX format).
  auto read_meta_field = [&](std::initializer_list<const char*> attr_names,
                             const char* child_element_name) -> std::string {
    for (const char* attr : attr_names) {
      if (std::string v = AttrOrEmpty(map_meta, attr); !v.empty()) {
        return v;
      }
    }
    if (const pugi::xml_node child = map_meta.child(child_element_name)) {
      return AttrOrEmpty(child, "value");
    }
    return {};
  };

  meta.title_ = read_meta_field({"title", "Title"}, "Title");
  meta.author_ = read_meta_field({"author", "Author"}, "InitialAuthor");
  meta.created_ = read_meta_field({"created", "creationDate", "Created"}, "CreationTime");
  return meta;
}

struct ZipArchiveCloser {
  zip_t* archive_ = nullptr;
  ZipArchiveCloser(const ZipArchiveCloser&) = delete;
  ZipArchiveCloser& operator=(const ZipArchiveCloser&) = delete;
  ~ZipArchiveCloser() {
    if (archive_ != nullptr) {
      zip_close(archive_);
    }
  }
};

[[nodiscard]] zip_int64_t LocateEntryByBasename(zip_t* archive, const char* basename) {
  const zip_int64_t entry_count = zip_get_num_entries(archive, 0);
  if (entry_count < 0) {
    return -1;
  }
  for (zip_int64_t i = 0; i < entry_count; ++i) {
    const char* name = zip_get_name(archive, static_cast<zip_uint64_t>(i), 0);
    if (name == nullptr) {
      continue;
    }
    const char* slash = std::strrchr(name, '/');
    const char* base = slash != nullptr ? slash + 1 : name;
    if (std::strcmp(base, basename) == 0) {
      return i;
    }
  }
  return -1;
}

[[nodiscard]] std::optional<std::string> ReadZipEntryUncompressed(zip_t* archive, zip_uint64_t index) {
  zip_stat_t stat{};
  if (zip_stat_index(archive, index, 0, &stat) != 0) {
    return std::nullopt;
  }
  if ((stat.valid & ZIP_STAT_SIZE) == 0) {
    return std::nullopt;
  }
  zip_file_t* file = zip_fopen_index(archive, index, 0);
  if (file == nullptr) {
    return std::nullopt;
  }
  std::string buffer(static_cast<size_t>(stat.size), '\0');
  zip_uint64_t read_total = 0;
  while (read_total < stat.size) {
    const zip_int64_t chunk =
        zip_fread(file, buffer.data() + read_total, stat.size - read_total);
    if (chunk <= 0) {
      zip_fclose(file);
      return std::nullopt;
    }
    read_total += static_cast<zip_uint64_t>(chunk);
  }
  zip_fclose(file);
  return {std::move(buffer)};
}

[[nodiscard]] zip_int64_t LocateImageEntry(zip_t* archive, const std::string& image_id) {
  const zip_int64_t idx = LocateEntryByBasename(archive, image_id.c_str());
  if (idx >= 0) {
    return idx;
  }
  const std::string with_png = image_id + ".png";
  return LocateEntryByBasename(archive, with_png.c_str());
}

}  // namespace

std::optional<ImxMindMapModel> LoadImxMindMapModelFromXml(const std::string_view data_xml,
                                                         const std::string_view mapmeta_xml) {
  std::optional<ImxMindMapModel> parsed = ParseDataXml(data_xml);
  if (!parsed) {
    return std::nullopt;
  }
  parsed->meta_ = ParseMapMetaXml(mapmeta_xml);
  return parsed;
}

std::optional<ImxMindMapModel> LoadImxMindMapModelFromFile(const std::string_view imx_path) {
  const std::string path{imx_path};
  int zip_error = 0;
  zip_t* raw = zip_open(path.c_str(), ZIP_RDONLY, &zip_error);
  const ZipArchiveCloser closer{raw};
  if (raw == nullptr) {
    return std::nullopt;
  }

  const zip_int64_t data_index = LocateEntryByBasename(raw, "data.xml");
  const zip_int64_t meta_index = LocateEntryByBasename(raw, "mapmeta.xml");
  if (data_index < 0 || meta_index < 0) {
    return std::nullopt;
  }

  std::optional<std::string> data_xml = ReadZipEntryUncompressed(raw, static_cast<zip_uint64_t>(data_index));
  std::optional<std::string> mapmeta_xml =
      ReadZipEntryUncompressed(raw, static_cast<zip_uint64_t>(meta_index));
  if (!data_xml || !mapmeta_xml) {
    return std::nullopt;
  }

  std::optional<ImxMindMapModel> model = LoadImxMindMapModelFromXml(*data_xml, *mapmeta_xml);
  if (!model) {
    return std::nullopt;
  }

  for (auto& node : model->nodes_) {
    if (node.image_asset_id_.empty()) {
      continue;
    }
    const zip_int64_t img_index = LocateImageEntry(raw, node.image_asset_id_);
    if (img_index < 0) {
      continue;
    }
    if (auto bytes = ReadZipEntryUncompressed(raw, static_cast<zip_uint64_t>(img_index))) {
      node.image_bytes_ = std::move(*bytes);
    }
  }

  return model;
}

}  // namespace mind_map::core
