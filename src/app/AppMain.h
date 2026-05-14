#pragma once

namespace mind_map::app {

/// Entry point: creates the GLFW window, initializes Dear ImGui, and runs the render loop until close.
/// Recognizes `--help` (in-app guide mind map, not CLI usage text). Any other argument is treated as an
/// optional startup document path (`.mmh` / `.imx`) when `--help` is absent.
[[nodiscard]] int RunApp(int argc, char** argv);

}  // namespace mind_map::app
