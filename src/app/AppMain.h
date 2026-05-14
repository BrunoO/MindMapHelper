#pragma once

#include <string_view>

namespace mind_map::app {

/// Entry point: creates the GLFW window, initializes Dear ImGui, and runs the render loop until close.
[[nodiscard]] int RunApp(std::string_view startup_path = {});

}  // namespace mind_map::app
