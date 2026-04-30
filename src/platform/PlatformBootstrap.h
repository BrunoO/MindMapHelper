#pragma once

struct GLFWwindow;

namespace mind_map::platform {

[[nodiscard]] GLFWwindow* CreateMainWindow(int width, int height, const char* title);
void DestroyMainWindow(GLFWwindow* window);
[[nodiscard]] const char* GetGlslVersion();
void ConfigurePlatformGlContextHints();

}  // namespace mind_map::platform
