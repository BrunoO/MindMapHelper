#include "platform/PlatformBootstrap.h"

#include "utils/Logger.h"

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

namespace {

void GlfwErrorCallback(int error, const char* description) {
  LOG_ERROR_BUILD("GLFW Error " << error << ": " << description);
}

}  // namespace

namespace mind_map::platform {

GLFWwindow* CreateMainWindow(int width, int height, const char* title) {
  glfwSetErrorCallback(GlfwErrorCallback);
  if (glfwInit() == 0) {
    return nullptr;
  }

  ConfigurePlatformGlContextHints();

  GLFWwindow* const window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (window == nullptr) {
    glfwTerminate();
    return nullptr;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  return window;
}

void DestroyMainWindow(GLFWwindow* window) {
  glfwDestroyWindow(window);
  glfwTerminate();
}

const char* GetGlslVersion() {
#if defined(IMGUI_IMPL_OPENGL_ES2)
  return "#version 100";
#elif defined(IMGUI_IMPL_OPENGL_ES3)
  return "#version 300 es";
#elif defined(__APPLE__)
  return "#version 150";
#else
  return "#version 130";
#endif
}

}  // namespace mind_map::platform
