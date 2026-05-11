#pragma once

#include "core/imx/ImxMindMapModel.h"

#include <optional>
#include <string_view>

namespace mind_map::core {

/// Read an `.imx` ZIP, extract `data.xml` and `mapmeta.xml`, and build an in-memory IMX model.
[[nodiscard]] std::optional<ImxMindMapModel> LoadImxMindMapModelFromFile(std::string_view imx_path);

/// Same pipeline without ZIP (for tests and tooling). `data_xml` / `mapmeta_xml` must be UTF-8.
[[nodiscard]] std::optional<ImxMindMapModel> LoadImxMindMapModelFromXml(std::string_view data_xml,
                                                                        std::string_view mapmeta_xml);

}  // namespace mind_map::core
