#include "platform/PlatformBootstrap.h"

#include "utils/Logger.h"
#include "utils/LoggingUtils.h"

#include <GLFW/glfw3.h>

#include <array>
#include <string>

#include <shellapi.h>

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

void OpenUrl(std::string_view url) {
  const int wide_len = MultiByteToWideChar(CP_UTF8, 0, url.data(),
                                           static_cast<int>(url.size()), nullptr, 0);
  if (wide_len <= 0) { return; }
  std::wstring wide_url(static_cast<size_t>(wide_len), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, url.data(), static_cast<int>(url.size()),
                      wide_url.data(), wide_len);
  ShellExecuteW(nullptr, L"open", wide_url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

}  // namespace mind_map::platform
