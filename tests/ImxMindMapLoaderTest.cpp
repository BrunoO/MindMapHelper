#include "core/imx/ImxMindMapLoader.h"

#include <zip.h>

#include <cassert>
#include <filesystem>
#include <string>

namespace {

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

void TestLoadFromXmlStrings() {
  using mind_map::core::LoadImxMindMapModelFromXml;
  const auto model = LoadImxMindMapModelFromXml(MakeMinimalDataXml(), MakeMinimalMapMetaXml());
  assert(model);
  assert(model->root_id_ == "root-1");
  assert(model->meta_.title_ == "Test map");
  assert(model->meta_.author_ == "Unit test");
  assert(model->meta_.created_ == "2026-05-11");
  assert(model->nodes_.size() == 2U);
  assert(model->nodes_[0].id_ == "root-1");
  assert(model->nodes_[0].text_ == "Root label");
  assert(model->nodes_[0].children_.size() == 1U);
  assert(model->nodes_[0].children_[0] == "child-1");
  assert(model->nodes_[1].id_ == "child-1");
  assert(model->nodes_[1].text_ == "Child label");
}

void TestLoadFromMinimalZipFile() {
  using mind_map::core::LoadImxMindMapModelFromFile;
  const std::filesystem::path zip_path =
      std::filesystem::temp_directory_path() / "mindmap_helper_imx_loader_test.zip";

  int zip_error = 0;
  zip_t* archive = zip_open(zip_path.string().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &zip_error);  // NOLINT(hicpp-signed-bitwise)
  assert(archive != nullptr);

  const std::string data_xml = MakeMinimalDataXml();
  const std::string mapmeta_xml = MakeMinimalMapMetaXml();

  zip_source_t* data_src = zip_source_buffer(archive, data_xml.data(), data_xml.size(), 0);
  assert(data_src != nullptr);
  assert(zip_file_add(archive, "data.xml", data_src, ZIP_FL_ENC_UTF_8) >= 0);

  zip_source_t* meta_src = zip_source_buffer(archive, mapmeta_xml.data(), mapmeta_xml.size(), 0);
  assert(meta_src != nullptr);
  assert(zip_file_add(archive, "mapmeta.xml", meta_src, ZIP_FL_ENC_UTF_8) >= 0);

  assert(zip_close(archive) == 0);

  const auto model = LoadImxMindMapModelFromFile(zip_path.string());
  assert(model);
  assert(model->root_id_ == "root-1");
  assert(model->nodes_.size() == 2U);

  static_cast<void>(std::filesystem::remove(zip_path));
}

}  // namespace

int main() {  // NOLINT(bugprone-exception-escape)
  TestLoadFromXmlStrings();
  TestLoadFromMinimalZipFile();
  return 0;
}
