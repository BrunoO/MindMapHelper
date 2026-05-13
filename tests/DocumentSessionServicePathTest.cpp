#include "app/DocumentSessionService.h"
#include "core/ImportService.h"
#include "core/JsonNativeDocumentRepository.h"
#include "core/MindMapDocument.h"

#include <zip.h>

#include <cassert>
#include <filesystem>
#include <string>

namespace {

[[nodiscard]] mind_map::core::MindMapDocument MinimalNativeDoc() {
  mind_map::core::MindMapDocument doc;
  mind_map::core::MindMapNode root;
  root.id_ = "r1";
  root.label_ = "R";
  doc.nodes_.push_back(std::move(root));
  mind_map::core::MindMapNodeLayout lay;
  lay.node_id_ = "r1";
  doc.layouts_.push_back(std::move(lay));
  return doc;
}

[[nodiscard]] std::filesystem::path WriteMinimalImxFile() {
  const std::filesystem::path zip_path =
      std::filesystem::temp_directory_path() / "mindmap_helper_document_session_path_test.imx";

  int zip_error = 0;
  zip_t* const archive =
      zip_open(zip_path.string().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &zip_error);  // NOLINT(hicpp-signed-bitwise)
  assert(archive != nullptr);

  const std::string data_xml = R"xml(<?xml version="1.0" encoding="UTF-8"?>
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

  const std::string mapmeta_xml = R"xml(<?xml version="1.0" encoding="UTF-8"?>
<meta>
  <MapMeta title="Test map" author="Unit test" created="2026-05-11"/>
</meta>
)xml";

  zip_source_t* const data_src = zip_source_buffer(archive, data_xml.data(), data_xml.size(), 0);
  assert(data_src != nullptr);
  assert(zip_file_add(archive, "data.xml", data_src, ZIP_FL_ENC_UTF_8) >= 0);

  zip_source_t* const meta_src = zip_source_buffer(archive, mapmeta_xml.data(), mapmeta_xml.size(), 0);
  assert(meta_src != nullptr);
  assert(zip_file_add(archive, "mapmeta.xml", meta_src, ZIP_FL_ENC_UTF_8) >= 0);

  assert(zip_close(archive) == 0);
  return zip_path;
}

void TestOpenFromPathNativeSetsPathAndClean() {
  mind_map::core::JsonNativeDocumentRepository repo;
  mind_map::app::DocumentSessionService session(repo);
  const mind_map::core::ImportService imports;

  const std::filesystem::path tmp =
      std::filesystem::temp_directory_path() / "mindmap_helper_document_session_path.mmh";
  assert(repo.Save(tmp.string(), MinimalNativeDoc()));

  mind_map::core::MindMapDocument doc;
  assert(session.OpenFromPath(tmp.string(), doc, imports));
  assert(session.HasPath());
  assert(!session.IsDirty());
  assert(doc.nodes_.size() == 1U);

  static_cast<void>(std::filesystem::remove(tmp));
}

void TestOpenFromPathImxClearsPathAndDirty() {
  mind_map::core::JsonNativeDocumentRepository repo;
  mind_map::app::DocumentSessionService session(repo);
  const mind_map::core::ImportService imports;

  const std::filesystem::path imx_path = WriteMinimalImxFile();
  mind_map::core::MindMapDocument doc;
  assert(session.OpenFromPath(imx_path.string(), doc, imports));
  assert(!session.HasPath());
  assert(session.IsDirty());
  assert(doc.nodes_.size() == 2U);

  static_cast<void>(std::filesystem::remove(imx_path));
}

}  // namespace

int main() {  // NOLINT(bugprone-exception-escape)
  TestOpenFromPathNativeSetsPathAndClean();
  TestOpenFromPathImxClearsPathAndDirty();
  return 0;
}
