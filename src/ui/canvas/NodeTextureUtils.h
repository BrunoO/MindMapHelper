#pragma once

#include "imgui.h"

#include <string_view>

namespace mind_map::ui {

struct PngTextureUpload {
  ImTextureID texture_id_ = 0;
  int width_px_ = 0;
  int height_px_ = 0;
};

/// Decodes PNG bytes and uploads a GL texture; returns empty texture_id on failure. Must be called from the GL thread.
[[nodiscard]] PngTextureUpload UploadPngTexture(std::string_view png_bytes,
                                                std::string_view context = {});

/// Deletes a GL texture created by UploadPngTexture. No-op when tex_id == 0. Must be called from the GL thread.
void ReleaseTexture(ImTextureID tex_id);

}  // namespace mind_map::ui
