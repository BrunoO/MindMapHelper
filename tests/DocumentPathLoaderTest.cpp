#include "core/DocumentPathLoader.h"
#include "core/ImportService.h"
#include "core/JsonNativeDocumentRepository.h"
#include "core/MindMapDocument.h"
#include "core/PathExtension.h"

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

[[nodiscard]] std::string MakeMinimalDataXml() {
  return R"xml(<?xml version="1.0" encoding="UTF-8"?>
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
}

[[nodiscard]] std::string MakeMinimalMapMetaXml() {
  return R"xml(<?xml version="1.0" encoding="UTF-8"?>
<meta>
  <MapMeta title="Test map" author="Unit test" created="2026-05-11"/>
</meta>
)xml";
}

[[nodiscard]] std::filesystem::path WriteMinimalImxFile() {
  const std::filesystem::path zip_path =
      std::filesystem::temp_directory_path() / "mindmap_helper_document_path_loader_test.imx";

  int zip_error = 0;
  zip_t* const archive =
      zip_open(zip_path.string().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &zip_error);  // NOLINT(hicpp-signed-bitwise)
  assert(archive != nullptr);

  const std::string data_xml = MakeMinimalDataXml();
  const std::string mapmeta_xml = MakeMinimalMapMetaXml();

  zip_source_t* const data_src = zip_source_buffer(archive, data_xml.data(), data_xml.size(), 0);
  assert(data_src != nullptr);
  assert(zip_file_add(archive, "data.xml", data_src, ZIP_FL_ENC_UTF_8) >= 0);

  zip_source_t* const meta_src = zip_source_buffer(archive, mapmeta_xml.data(), mapmeta_xml.size(), 0);
  assert(meta_src != nullptr);
  assert(zip_file_add(archive, "mapmeta.xml", meta_src, ZIP_FL_ENC_UTF_8) >= 0);

  assert(zip_close(archive) == 0);
  return zip_path;
}

void TestLowercaseExtensionOf() {
  using mind_map::core::LowercaseExtensionOf;
  assert(LowercaseExtensionOf("/a/B/C.File.MMH") == ".mmh");
  assert(LowercaseExtensionOf("nope").empty());
}

void TestNativeMmh() {
  using mind_map::core::DocumentPathLoadOutcome;
  const std::filesystem::path tmp =
      std::filesystem::temp_directory_path() / "mindmap_helper_document_path_loader_native.mmh";
  mind_map::core::JsonNativeDocumentRepository repo;
  const mind_map::core::ImportService imports;
  assert(repo.Save(tmp.string(), MinimalNativeDoc()));

  const auto r = mind_map::core::LoadMindMapFromPath(tmp.string(), repo, imports);
  assert(r.outcome_ == DocumentPathLoadOutcome::NativeAtPath);
  assert(r.doc_.nodes_.size() == 1U);

  static_cast<void>(std::filesystem::remove(tmp));
}

void TestImxPath() {
  using mind_map::core::DocumentPathLoadOutcome;
  mind_map::core::JsonNativeDocumentRepository repo;
  const mind_map::core::ImportService imports;
  const std::filesystem::path imx_path = WriteMinimalImxFile();

  const auto r = mind_map::core::LoadMindMapFromPath(imx_path.string(), repo, imports);
  assert(r.outcome_ == DocumentPathLoadOutcome::ImportedNoPath);
  assert(r.doc_.nodes_.size() == 2U);

  static_cast<void>(std::filesystem::remove(imx_path));
}

void TestUnsupportedExtension() {
  using mind_map::core::DocumentPathLoadOutcome;
  mind_map::core::JsonNativeDocumentRepository repo;
  const mind_map::core::ImportService imports;
  const auto r = mind_map::core::LoadMindMapFromPath("/tmp/mindmap_helper_dpl_unknown.bin", repo, imports);
  assert(r.outcome_ == DocumentPathLoadOutcome::Failed);
}

}  // namespace

int main() {  // NOLINT(bugprone-exception-escape)
  TestLowercaseExtensionOf();
  TestNativeMmh();
  TestImxPath();
  TestUnsupportedExtension();
  return 0;
}
