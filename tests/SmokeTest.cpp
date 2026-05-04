#include <string_view>

int main() {
  constexpr std::string_view kProjectName = "MindMap Helper";
  static_assert(!kProjectName.empty(), "Project name must not be empty");
  return 0;
}
