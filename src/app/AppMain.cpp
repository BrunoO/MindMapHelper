#include "app/AppMain.h"

#include "platform/PlatformBootstrap.h"
#include "ui/MindMapUi.h"
#include "ui/UiState.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif  // _MSC_VER && (_MSC_VER >= 1900) && !IMGUI_DISABLE_WIN32_FUNCTIONS

namespace mind_map::app {

int RunApp() {
  mind_map::ui::UiState ui_state;
  GLFWwindow* const window = mind_map::platform::CreateMainWindow(1280, 720, "MindMap Helper");
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

  constexpr ImVec4 kClearColor{0.11F, 0.11F, 0.14F, 1.0F};
  constexpr int kIconifiedSleepMs = 10;

  while (glfwWindowShouldClose(window) == 0) {
    glfwPollEvents();
    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
      ImGui_ImplGlfw_Sleep(kIconifiedSleepMs);
      continue;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    mind_map::ui::RenderMainUi(ui_state);

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
