#include "platform/PlatformBootstrap.h"

#include "utils/Logger.h"
#include "utils/LoggingUtils.h"

#include <GLFW/glfw3.h>

#include <array>
#include <string>

namespace mind_map::platform {

void ConfigurePlatformGlContextHints() {
#if defined(IMGUI_IMPL_OPENGL_ES2)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#else
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
}

void LaunchNewWindow(std::string_view path) {
  std::array<char, MAX_PATH> exe_buf{};
  if (GetModuleFileNameA(nullptr, exe_buf.data(), MAX_PATH) == 0) {
    logging_utils::LogWindowsApiError("GetModuleFileNameA", "", GetLastError());
    return;
  }

  std::string cmd = "\"";
  cmd += exe_buf.data();
  cmd += "\"";
  if (!path.empty()) {
    cmd += " \"";
    cmd += std::string(path);
    cmd += "\"";
  }

  STARTUPINFOA si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  if (CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi) != 0) {
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  } else {
    logging_utils::LogWindowsApiError("CreateProcessA", path, GetLastError());
  }
}

}  // namespace mind_map::platform
