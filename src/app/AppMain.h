#pragma once

#include <string_view>

namespace mind_map::app {

[[nodiscard]] int RunApp(std::string_view startup_path = {});

}  // namespace mind_map::app
