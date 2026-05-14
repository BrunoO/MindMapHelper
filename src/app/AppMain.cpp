#include "app/AppMain.h"

#include "app/DocumentSessionService.h"
#include "core/ImportService.h"
#include "core/JsonNativeDocumentRepository.h"
#include "platform/PlatformBootstrap.h"
#include "ui/MindMapUi.h"
#include "ui/UiState.h"
#include "ui/commands/CommandHistory.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

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

}  // namespace

int RunApp(std::string_view startup_path) {
  mind_map::ui::UiState ui_state;
  mind_map::ui::commands::CommandHistory history;
  mind_map::core::JsonNativeDocumentRepository repo;
  mind_map::app::DocumentSessionService session(repo);
  const mind_map::core::ImportService import_service;

  // LoadFrom touches OpenGL (node textures). Defer until after the GL context exists — same order as
  // File → Open / Import in the main loop (ImGui + GL are initialized first).
  std::optional<mind_map::core::MindMapDocument> deferred_startup_document;
  if (!startup_path.empty()) {
    mind_map::core::MindMapDocument doc;
    if (session.OpenFromPath(startup_path, doc, import_service)) {
      deferred_startup_document = std::move(doc);
    }
  } else {
    mind_map::core::MindMapDocument doc;
    session.New(doc);
    deferred_startup_document = std::move(doc);
  }

  const std::string initial_title = WindowTitleForPath(session.GetCurrentPath());
  GLFWwindow* const window = mind_map::platform::CreateMainWindow(1280, 720, initial_title.c_str());
  if (window == nullptr) {
    return 1;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // NOLINT(hicpp-signed-bitwise)

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

    if (session.GetCurrentPath() != last_path) {
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
  return 0;
}

}  // namespace mind_map::app
