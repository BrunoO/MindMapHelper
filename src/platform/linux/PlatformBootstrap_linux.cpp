#include "platform/PlatformBootstrap.h"

#include <GLFW/glfw3.h>

#include <climits>
#include <string>
#include <vector>

#include <spawn.h>
#include <unistd.h>

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
  std::array<char, PATH_MAX> buf{};
  const ssize_t len = readlink("/proc/self/exe", buf.data(), buf.size() - 1U);
  if (len <= 0) { return; }
  buf[static_cast<size_t>(len)] = '\0';

  std::string exe(buf.data());
  std::string path_arg(path);

  std::vector<char*> argv;
  argv.push_back(exe.data());
  if (!path.empty()) { argv.push_back(path_arg.data()); }
  argv.push_back(nullptr);

  pid_t pid = 0;
  (void)posix_spawn(&pid, exe.c_str(), nullptr, nullptr, argv.data(), environ);
}

}  // namespace mind_map::platform
