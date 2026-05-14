#pragma once

#include <string_view>

struct GLFWwindow;

namespace mind_map::platform {

/// Creates the main GLFW window with a GL context; must be called once from AppMain before the render loop.
[[nodiscard]] GLFWwindow* CreateMainWindow(int width, int height, const char* title);
/// Destroys the GLFW window and its GL context; call after the render loop exits.
void DestroyMainWindow(GLFWwindow* window);
/// Returns the GLSL version string required by the Dear ImGui OpenGL3 backend (e.g. "#version 150").
[[nodiscard]] const char* GetGlslVersion();
/// Sets platform-specific GL context hints before window creation (e.g. forward-compat core profile on macOS).
void ConfigurePlatformGlContextHints();
/// Spawns a new instance of this executable as a detached process.
/// Pass a non-empty path to open that file; pass an empty string for a blank new map.
void LaunchNewWindow(std::string_view path);

}  // namespace mind_map::platform
