#include <string_view>

int main() {
  constexpr std::string_view kProjectName = "MindMap Helper";
  if (kProjectName.empty()) {
    return 1;
  }
  return 0;
}
