#pragma once

#include "imgui.h"

#include <string_view>

namespace mind_map::ui {

/// Decodes PNG bytes and uploads a GL texture; returns 0 on failure. Must be called from the GL thread.
[[nodiscard]] ImTextureID UploadPngTexture(std::string_view png_bytes,
                                          std::string_view context = {});

/// Deletes a GL texture created by UploadPngTexture. No-op when tex_id == 0. Must be called from the GL thread.
void ReleaseTexture(ImTextureID tex_id);

}  // namespace mind_map::ui
