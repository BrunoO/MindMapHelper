#include <string_view>

int main() {
  constexpr std::string_view kProjectName = "bMindMap";
  if (kProjectName.empty()) {
    return 1;
  }
  return 0;
}
