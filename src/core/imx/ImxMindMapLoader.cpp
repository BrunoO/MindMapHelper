#include "core/imx/ImxMindMapLoader.h"

#include <pugixml.hpp>
#include <zip.h>

#include <algorithm>
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

[[nodiscard]] std::optional<std::string> ExtractCellTextFromPropertyNode(const pugi::xml_node& node) {
  const pugi::xml_attribute key_attr = node.attribute("key");
  if (key_attr.empty() || std::string_view{key_attr.value()} != kGaiaCellTextKey) {
    return std::nullopt;
  }
  if (const pugi::xml_attribute value_attr = node.attribute("value")) {
    return std::string{value_attr.value()};
  }
  return std::string{node.text().get()};
}

[[nodiscard]] std::string ExtractCellText(const pugi::xml_node& node_with_id) {
  std::vector<pugi::xml_node> stack;
  stack.push_back(node_with_id);
  while (!stack.empty()) {
    const pugi::xml_node node = stack.back();
    stack.pop_back();
    if (node.type() == pugi::node_element && std::strcmp(node.name(), "property") == 0) {
      if (auto result = ExtractCellTextFromPropertyNode(node)) {
        return *result;
      }
    }
    for (pugi::xml_node child = node.last_child(); !child.empty(); child = child.previous_sibling()) {
      stack.push_back(child);
    }
  }
  return {};
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

[[nodiscard]] std::vector<std::pair<std::string, std::string>> CollectBranches(const pugi::xml_document& doc) {
  std::vector<std::pair<std::string, std::string>> edges;
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
        edges.emplace_back(source, target);
      }
    }
    for (pugi::xml_node child = node.last_child(); !child.empty(); child = child.previous_sibling()) {
      stack.push_back(child);
    }
  }
  return edges;
}

[[nodiscard]] std::unordered_map<std::string, std::vector<std::string>> BuildChildrenMap(
    const std::vector<std::pair<std::string, std::string>>& edges) {
  std::unordered_map<std::string, std::vector<std::string>> children;
  for (const auto& [parent, child] : edges) {
    std::vector<std::string>& bucket = children[parent];
    if (std::find(bucket.begin(), bucket.end(), child) == bucket.end()) {  // NOLINT(llvm-use-ranges)
      bucket.push_back(child);
    }
  }
  return children;
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

  const std::unordered_map<std::string, std::string> id_to_text = BuildIdToTextMap(doc);
  const std::vector<std::pair<std::string, std::string>> edges = CollectBranches(doc);
  const std::unordered_map<std::string, std::vector<std::string>> children_by_parent = BuildChildrenMap(edges);

  std::unordered_set<std::string> all_ids;
  all_ids.insert(root_id);
  for (const auto& [node_id, node_text] : id_to_text) {
    static_cast<void>(node_text);
    all_ids.insert(node_id);
  }
  for (const auto& [src, tgt] : edges) {
    all_ids.insert(src);
    all_ids.insert(tgt);
  }

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
    if (const auto it = id_to_text.find(id); it != id_to_text.end()) {
      node.text_ = it->second;
    }
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
    if (const auto it = id_to_text.find(id); it != id_to_text.end()) {
      node.text_ = it->second;
    }
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
  meta.title_ = AttrOrEmpty(map_meta, "title");
  if (meta.title_.empty()) {
    meta.title_ = AttrOrEmpty(map_meta, "Title");
  }
  meta.author_ = AttrOrEmpty(map_meta, "author");
  if (meta.author_.empty()) {
    meta.author_ = AttrOrEmpty(map_meta, "Author");
  }
  meta.created_ = AttrOrEmpty(map_meta, "created");
  if (meta.created_.empty()) {
    meta.created_ = AttrOrEmpty(map_meta, "creationDate");
  }
  if (meta.created_.empty()) {
    meta.created_ = AttrOrEmpty(map_meta, "Created");
  }
  return meta;
}

struct ZipArchiveCloser {
  zip_t* archive_ = nullptr;
  ZipArchiveCloser() = default;
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

  return LoadImxMindMapModelFromXml(*data_xml, *mapmeta_xml);
}

}  // namespace mind_map::core
