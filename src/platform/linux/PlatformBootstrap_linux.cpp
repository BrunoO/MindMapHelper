#include "platform/PlatformBootstrap.h"

#include "utils/LogFormatUtils.h"
#include "utils/Logger.h"

#include <GLFW/glfw3.h>

#include <array>
#include <climits>
#include <cstring>
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
  if (len <= 0) {
    LOG_ERROR("LaunchNewWindow: readlink /proc/self/exe failed");
    return;
  }
  buf[static_cast<size_t>(len)] = '\0';

  std::string exe(buf.data());
  std::string path_arg(path);

  std::vector<char*> argv;
  argv.push_back(exe.data());
  if (!path.empty()) { argv.push_back(path_arg.data()); }
  argv.push_back(nullptr);

  pid_t pid = 0;
  if (const int spawn_err = posix_spawn(&pid, exe.c_str(), nullptr, nullptr, argv.data(), nullptr);
      spawn_err != 0) {
    LOG_ERROR_BUILD("LaunchNewWindow: posix_spawn failed for '" << path << "': "
                    << ThreadSafeStrerror(spawn_err));
  }
}

}  // namespace mind_map::platform
