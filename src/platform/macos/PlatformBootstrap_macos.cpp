#include "platform/PlatformBootstrap.h"

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

#include <climits>
#include <string>
#include <vector>

#include <crt_externs.h>
#include <mach-o/dyld.h>
#include <spawn.h>

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
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
}

void LaunchNewWindow(std::string_view path) {
  constexpr uint32_t kBufSize = PATH_MAX;
  std::array<char, kBufSize> buf{};
  if (uint32_t size = kBufSize; _NSGetExecutablePath(buf.data(), &size) != 0) {
    return;
  }

  std::string exe(buf.data());
  std::string path_arg(path);

  std::vector<char*> argv;
  argv.push_back(exe.data());
  if (!path.empty()) { argv.push_back(path_arg.data()); }
  argv.push_back(nullptr);

  pid_t pid = 0;
  (void)posix_spawn(&pid, exe.c_str(), nullptr, nullptr, argv.data(), *_NSGetEnviron());
}

}  // namespace mind_map::platform
