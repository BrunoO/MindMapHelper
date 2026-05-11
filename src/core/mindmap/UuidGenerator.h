#pragma once

#include <string>

namespace mind_map::core::mindmap {

// Returns a random UUID v4 string: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
[[nodiscard]] std::string GenerateUuidV4();

}  // namespace mind_map::core::mindmap
