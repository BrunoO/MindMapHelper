#include "app/AppMain.h"

#include "utils/ExceptionHandling.h"
#include "utils/Logger.h"

#include "app/DocumentSessionService.h"
#include "app/HelpMindMapDocument.h"
#include "core/ImportService.h"
#include "core/JsonNativeDocumentRepository.h"
#include "platform/PlatformBootstrap.h"
#include "ui/MindMapUi.h"
#include "ui/UiState.h"
#include "ui/commands/CommandHistory.h"

#include "ui/canvas/InlineMarkup.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "misc/freetype/imgui_freetype.h"

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif  // _MSC_VER && (_MSC_VER >= 1900) && !IMGUI_DISABLE_WIN32_FUNCTIONS

namespace mind_map::app {

namespace {

[[nodiscard]] std::string FilenameFromPath(std::string_view path) {
  const auto sep = path.find_last_of("/\\");
  return std::string{sep == std::string_view::npos ? path : path.substr(sep + 1)};
}

[[nodiscard]] std::string WindowTitleForPath(std::string_view path) {
  return path.empty() ? "MindMap Helper"
                      : FilenameFromPath(path) + " - MindMap Helper";
}

struct StartupCli {
  bool help_map_ = false;
  std::string path_;
};

[[nodiscard]] StartupCli ParseStartupCli(int argc, char** argv) {
  StartupCli out;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg{argv[i]};
    if (arg == "--help") {
      out.help_map_ = true;
      continue;
    }
    if (out.path_.empty() && !arg.empty()) {
      out.path_ = std::string{arg};
    }
  }
  return out;
}

}  // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity) -- GLFW/ImGui bootstrap and main loop
[[nodiscard]] int RunAppBody(int argc, char** argv) {
  const StartupCli cli = ParseStartupCli(argc, argv);
  mind_map::ui::UiState ui_state;
  mind_map::ui::commands::CommandHistory history;
  mind_map::core::JsonNativeDocumentRepository repo;
  mind_map::app::DocumentSessionService session(repo);
  const mind_map::core::ImportService import_service;

  // LoadFrom touches OpenGL (node textures). Defer until after the GL context exists — same order as
  // File → Open / Import in the main loop (ImGui + GL are initialized first).
  std::optional<mind_map::core::MindMapDocument> deferred_startup_document;
  if (cli.help_map_) {
    mind_map::core::MindMapDocument scratch;
    session.New(scratch);
    deferred_startup_document = BuildHelpMindMapDocument();
  } else if (!cli.path_.empty()) {
    mind_map::core::MindMapDocument doc;
    if (session.OpenFromPath(cli.path_, doc, import_service)) {
      deferred_startup_document = std::move(doc);
      LOG_INFO_BUILD("RunApp: opened startup document '" << cli.path_ << '\'');
    } else {
      LOG_WARNING_BUILD("RunApp: failed to open startup document '" << cli.path_ << '\'');
    }
  } else {
    mind_map::core::MindMapDocument doc;
    session.New(doc);
    deferred_startup_document = std::move(doc);
  }

  const std::string initial_title =
      cli.help_map_ ? "Help — MindMap Helper" : WindowTitleForPath(session.GetCurrentPath());
  GLFWwindow* const window = mind_map::platform::CreateMainWindow(1280, 720, initial_title.c_str());
  if (window == nullptr) {
    LOG_ERROR("RunApp: failed to create main window");
    return 1;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // NOLINT(hicpp-signed-bitwise)

  constexpr float kXScaleMin    = 0.5F;
  constexpr float kXScaleMax    = 4.0F;
  constexpr float kFontSizeBase = 15.0F;
  constexpr float kFontSizeMin  = 8.0F;
  constexpr float kFontSizeMax  = 72.0F;

  float xscale = 1.0F;
  glfwGetWindowContentScale(window, &xscale, nullptr);
  xscale = (std::max)(kXScaleMin, (std::min)(xscale, kXScaleMax));
  const float font_size =
      (std::max)(kFontSizeMin, (std::min)(std::floor(kFontSizeBase * xscale), kFontSizeMax));
  // Null-terminated pair array: each pair is [first, last] codepoint; 0 ends the list.
  static const std::array<ImWchar, 7> kGlyphRanges = {
    0x0020, 0x00FF,  // Basic Latin + Latin Supplement
    0x25B6, 0x25B6,  // ▶ BLACK RIGHT-POINTING TRIANGLE
    0x25BC, 0x25BC,  // ▼ BLACK DOWN-POINTING TRIANGLE
    0,
  };
  if (const ImFont* const loaded_font =
          io.Fonts->AddFontFromFileTTF("assets/fonts/Inter-Regular.ttf", font_size, nullptr, kGlyphRanges.data());
      loaded_font == nullptr) {
    LOG_WARNING_BUILD("RunApp: failed to load 'assets/fonts/Inter-Regular.ttf' (size "
                      << font_size << "px) — using ImGui default font");
    io.Fonts->AddFontDefault();
    io.FontGlobalScale = 1.0F;
    // No styled variants available; markup degrades to regular font for bold/italic.
    mind_map::ui::canvas::InitInlineMarkupFonts({});
  } else {
    // Load FreeType synthetic bold, italic, and bold-italic from the same Inter-Regular.ttf.
    // No extra font files needed; ImGuiFreeTypeBuilderFlags synthesises the variants.
    ImFontConfig bold_cfg;
    bold_cfg.FontBuilderFlags = static_cast<unsigned int>(ImGuiFreeTypeBuilderFlags_Bold);
    ImFont* const bold_font = io.Fonts->AddFontFromFileTTF(
        "assets/fonts/Inter-Regular.ttf", font_size, &bold_cfg, kGlyphRanges.data());

    ImFontConfig italic_cfg;
    italic_cfg.FontBuilderFlags = static_cast<unsigned int>(ImGuiFreeTypeBuilderFlags_Oblique);
    ImFont* const italic_font = io.Fonts->AddFontFromFileTTF(
        "assets/fonts/Inter-Regular.ttf", font_size, &italic_cfg, kGlyphRanges.data());

    ImFontConfig bold_italic_cfg;
    bold_italic_cfg.FontBuilderFlags = static_cast<unsigned int>(
        ImGuiFreeTypeBuilderFlags_Bold | ImGuiFreeTypeBuilderFlags_Oblique);
    ImFont* const bold_italic_font = io.Fonts->AddFontFromFileTTF(
        "assets/fonts/Inter-Regular.ttf", font_size, &bold_italic_cfg, kGlyphRanges.data());

    mind_map::ui::canvas::InitInlineMarkupFonts({bold_font, italic_font, bold_italic_font});
    io.FontGlobalScale = 1.0F / xscale;  // undo pixel scaling so layout sizes stay in logical px
  }

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(mind_map::platform::GetGlslVersion());

  if (deferred_startup_document.has_value()) {
    ui_state.canvas_.LoadFrom(*deferred_startup_document);
    ui_state.ApplyViewport(deferred_startup_document->viewport_);
    deferred_startup_document.reset();
  }

  constexpr ImVec4 kClearColor{0.11F, 0.11F, 0.14F, 1.0F};
  constexpr int kIconifiedSleepMs = 10;
  std::string last_path{session.GetCurrentPath()};
  const bool fixed_help_window_title = cli.help_map_;

  while (glfwWindowShouldClose(window) == 0) {
    glfwPollEvents();

    // Intercept GLFW close when there are unsaved changes.
    if (glfwWindowShouldClose(window) != 0 && session.IsDirty()) {
      glfwSetWindowShouldClose(window, 0);
      session.RequestClose();
    }

    // The close guard modal (rendered by RenderMainUi) sets IsCloseConfirmed on
    // Save/Discard; break here so the cleanup path runs correctly.
    if (session.IsCloseConfirmed()) {
      break;
    }

    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
      ImGui_ImplGlfw_Sleep(kIconifiedSleepMs);
      continue;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    mind_map::ui::RenderMainUi(ui_state, session, history);

    if (ui_state.pending_launch_path_.has_value()) {
      mind_map::platform::LaunchNewWindow(*ui_state.pending_launch_path_);
      ui_state.pending_launch_path_.reset();
    }

    if (!fixed_help_window_title && session.GetCurrentPath() != last_path) {
      last_path = session.GetCurrentPath();
      glfwSetWindowTitle(window, WindowTitleForPath(last_path).c_str());
    }

    ImGui::Render();
    int display_w = 0;
    int display_h = 0;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(kClearColor.x * kClearColor.w, kClearColor.y * kClearColor.w, kClearColor.z * kClearColor.w,
                 kClearColor.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  mind_map::platform::DestroyMainWindow(window);
  LOG_IMPORTANT_BUILD("MindMap Helper exiting normally");
  return 0;
}

int RunApp(int argc, char** argv) {
  exception_handling::InstallTerminateHandler();
  LOG_IMPORTANT_BUILD("MindMap Helper starting");

  return exception_handling::RunFatal(
      "RunApp", [&]() { return RunAppBody(argc, argv); }, 1);
}

}  // namespace mind_map::app
