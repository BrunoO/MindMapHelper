#include "ui/canvas/NodeTextureUtils.h"

#include "utils/Logger.h"

#if defined(__APPLE__)
#  define GL_SILENCE_DEPRECATION
#  include <OpenGL/gl3.h>
#elif defined(_WIN32)
#  include <windows.h>
#  include <GL/GL.h>
#else
#  include <GL/gl.h>
#endif

// stb_image: ImGuiFileDialog only emits the implementation under USE_THUMBNAILS.
// We compile it here; use DONT_DEFINE_AGAIN guard in case thumbnails are later enabled.
#ifndef DONT_DEFINE_AGAIN__STB_IMAGE_IMPLEMENTATION
#  define STB_IMAGE_IMPLEMENTATION
#endif
#include "stb/stb_image.h"

#include <cstdint>
#include <string_view>

namespace mind_map::ui {

PngTextureUpload UploadPngTexture(std::string_view png_bytes, const std::string_view context) {
  if (png_bytes.empty()) {
    return {};
  }
  int width = 0;
  int height = 0;
  int channels = 0;
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* raw_ptr = reinterpret_cast<const stbi_uc*>(png_bytes.data());
  stbi_uc* pixels = stbi_load_from_memory(raw_ptr,
      static_cast<int>(png_bytes.size()),
      &width, &height, &channels, 4);
  if (pixels == nullptr || width <= 0 || height <= 0) {
    stbi_image_free(pixels);
    if (context.empty()) {
      LOG_WARNING_BUILD("UploadPngTexture: PNG decode failed (" << png_bytes.size() << " bytes)");
    } else {
      LOG_WARNING_BUILD("UploadPngTexture: PNG decode failed for " << context << " ("
                      << png_bytes.size() << " bytes)");
    }
    return {};
  }
  GLuint tex = 0U;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  stbi_image_free(pixels);
  return {static_cast<ImTextureID>(tex), width, height};
}

void ReleaseTexture(ImTextureID tex_id) {
  if (tex_id == 0) {
    return;
  }
  const auto tex = static_cast<GLuint>(tex_id);
  glDeleteTextures(1, &tex);
}

}  // namespace mind_map::ui
