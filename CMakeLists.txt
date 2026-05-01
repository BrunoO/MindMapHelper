cmake_minimum_required(VERSION 3.16)
project(bMindMap LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(BM_ENABLE_TESTS "Enable bMindMap tests" ON)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
if(APPLE)
  set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.9
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_DIR "${imgui_SOURCE_DIR}")

set(IMGUI_SOURCES
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

set(BM_UI_SOURCES
  src/ui/MindMapUi.cpp
  src/ui/demos/DemoBezierMindMap.cpp
  src/ui/demos/DemoStraightMindMap.cpp
  src/ui/demos/DemoTaperOrganicMindMap.cpp
  src/ui/demos/DemoRegistry.cpp
)

set(BM_PLATFORM_SOURCES
  src/platform/PlatformBootstrapCommon.cpp
)

if(WIN32)
  list(APPEND BM_PLATFORM_SOURCES src/platform/windows/PlatformBootstrap_windows.cpp)
elseif(APPLE)
  list(APPEND BM_PLATFORM_SOURCES src/platform/macos/PlatformBootstrap_macos.cpp)
elseif(UNIX)
  list(APPEND BM_PLATFORM_SOURCES src/platform/linux/PlatformBootstrap_linux.cpp)
endif()

add_library(bmindmap_core INTERFACE)

add_library(bmindmap_platform STATIC
  ${BM_PLATFORM_SOURCES}
)

add_library(bmindmap_ui STATIC
  ${BM_UI_SOURCES}
  ${IMGUI_SOURCES}
)

target_include_directories(bmindmap_platform PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_include_directories(bmindmap_ui PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${IMGUI_DIR}"
  "${IMGUI_DIR}/backends"
)

target_link_libraries(bmindmap_ui PRIVATE glfw)

target_link_libraries(bmindmap_platform PRIVATE glfw)

if(WIN32)
  target_link_libraries(bmindmap_platform PRIVATE opengl32)
elseif(APPLE)
  target_link_libraries(bmindmap_platform PRIVATE
    "-framework OpenGL"
  )
elseif(UNIX)
  find_package(OpenGL REQUIRED)
  target_link_libraries(bmindmap_platform PRIVATE OpenGL::GL)
endif()

add_executable(bmindmap
  src/main.cpp
  src/app/AppMain.cpp
)

target_include_directories(bmindmap PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${IMGUI_DIR}"
  "${IMGUI_DIR}/backends"
)

target_link_libraries(bmindmap PRIVATE
  bmindmap_core
  bmindmap_platform
  bmindmap_ui
  glfw
)

if(MSVC)
  target_compile_definitions(bmindmap_platform PRIVATE _CRT_SECURE_NO_WARNINGS)
  target_compile_definitions(bmindmap PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if(BM_ENABLE_TESTS)
  enable_testing()

  add_executable(bmindmap_smoke_test
    tests/SmokeTest.cpp
  )

  target_include_directories(bmindmap_smoke_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
  )

  target_link_libraries(bmindmap_smoke_test PRIVATE
    bmindmap_core
  )

  add_test(
    NAME bmindmap_smoke_test
    COMMAND bmindmap_smoke_test
  )
endif()
