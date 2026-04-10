cmake_minimum_required(VERSION 3.16)
project(bMindMap LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_PGO "Enable Profile-Guided Optimization (MSVC Release)" OFF)

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

add_executable(bmindmap
  src/main.cpp
  src/ui/MindMapUi.cpp
  ${IMGUI_SOURCES}
)

target_include_directories(bmindmap PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${IMGUI_DIR}"
  "${IMGUI_DIR}/backends"
)

target_link_libraries(bmindmap PRIVATE glfw)

if(WIN32)
  target_link_libraries(bmindmap PRIVATE opengl32)
elseif(APPLE)
  target_link_libraries(bmindmap PRIVATE
    "-framework OpenGL"
  )
elseif(UNIX)
  find_package(OpenGL REQUIRED)
  target_link_libraries(bmindmap PRIVATE OpenGL::GL)
endif()

if(MSVC)
  target_compile_definitions(bmindmap PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# PGO: keep dependency objects consistent with the main target (avoids LNK1269-style mismatches).
if(MSVC AND ENABLE_PGO)
  set(_bm_pgo_compile "$<$<CONFIG:Release>:/GL;/Gy>")
  set(_bm_pgo_link "$<$<CONFIG:Release>:/LTCG>")
  target_compile_options(bmindmap PRIVATE ${_bm_pgo_compile})
  target_link_options(bmindmap PRIVATE ${_bm_pgo_link})
  if(TARGET glfw)
    target_compile_options(glfw PRIVATE ${_bm_pgo_compile})
    target_link_options(glfw PRIVATE ${_bm_pgo_link})
  endif()
endif()
