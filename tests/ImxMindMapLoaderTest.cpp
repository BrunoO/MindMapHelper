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

// Real IMX files store node labels on the branch edge element, not on the branchNode target.
void TestBranchEdgeTextUsedAsNodeLabel() {
  using mind_map::core::LoadImxMindMapModelFromXml;
  const std::string data_xml = R"xml(<?xml version="1.0" encoding="UTF-8"?>
<mmGraphModel>
  <floatingIdea id="root-1">
    <property key="com.thinkbuzan.gaia.cell.text" value="Central Idea"/>
  </floatingIdea>
  <branch edge="1" id="edge-1" source="root-1" target="node-1">
    <property key="com.thinkbuzan.gaia.cell.text" value="Branch A"/>
  </branch>
  <branchNode id="node-1"/>
  <branch edge="1" id="edge-2" source="node-1" target="node-2">
    <property key="com.thinkbuzan.gaia.cell.text" value="Branch B"/>
  </branch>
  <branchNode id="node-2"/>
</mmGraphModel>
)xml";
  const auto model = LoadImxMindMapModelFromXml(data_xml, "");
  assert(model);
  assert(model->root_id_ == "root-1");
  assert(model->nodes_.size() == 3U);
  assert(model->nodes_[0].text_ == "Central Idea");
  assert(model->nodes_[1].text_ == "Branch A");
  assert(model->nodes_[2].text_ == "Branch B");
  assert(model->nodes_[0].children_.size() == 1U);
  assert(model->nodes_[0].children_[0] == "node-1");
  assert(model->nodes_[1].children_.size() == 1U);
  assert(model->nodes_[1].children_[0] == "node-2");
}

void TestMapMetaChildElements() {
  using mind_map::core::LoadImxMindMapModelFromXml;
  const std::string mapmeta_xml = R"xml(<?xml version="1.0" encoding="UTF-8"?>
<meta>
  <MapMeta>
    <Title value="Real Map Title"/>
    <InitialAuthor value="real.author"/>
    <CreationTime value="2022/03/07 17:50:45"/>
  </MapMeta>
</meta>
)xml";
  const std::string data_xml = R"xml(<?xml version="1.0" encoding="UTF-8"?>
<root><floatingIdea id="r"/></root>)xml";
  const auto model = LoadImxMindMapModelFromXml(data_xml, mapmeta_xml);
  assert(model);
  assert(model->meta_.title_ == "Real Map Title");
  assert(model->meta_.author_ == "real.author");
  assert(model->meta_.created_ == "2022/03/07 17:50:45");
}

void TestHtmlLabelExtraction() {
  using mind_map::core::LoadImxMindMapModelFromXml;
  const std::string data_xml = R"xml(<?xml version="1.0" encoding="UTF-8"?>
<root>
  <floatingIdea id="root-1">
    <property key="com.thinkbuzan.gaia.cell.text" value="Central Idea"/>
  </floatingIdea>
  <branch source="root-1" target="child-1"/>
  <idea id="child-1">
    <property key="com.thinkbuzan.gaia.entities.HTMLLabel">
      <HTMLLabel><![CDATA[<html><body><p>Child via CDATA</p></body></html>]]></HTMLLabel>
    </property>
  </idea>
  <branch source="root-1" target="child-2"/>
  <idea id="child-2">
    <property key="com.thinkbuzan.gaia.entities.HTMLLabel">
      <HTMLLabel><html><body><p>Child via XML</p></body></html></HTMLLabel>
    </property>
  </idea>
</root>
)xml";
  const auto model = LoadImxMindMapModelFromXml(data_xml, "");
  assert(model);
  assert(model->nodes_.size() == 3U);
  assert(model->nodes_[0].text_ == "Central Idea");
  assert(model->nodes_[1].text_ == "Child via CDATA");
  assert(model->nodes_[2].text_ == "Child via XML");
}

}  // namespace

int main() {  // NOLINT(bugprone-exception-escape)
  TestLoadFromXmlStrings();
  TestLoadFromMinimalZipFile();
  TestBranchEdgeTextUsedAsNodeLabel();
  TestMapMetaChildElements();
  TestHtmlLabelExtraction();
  return 0;
}
