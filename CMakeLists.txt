cmake_minimum_required(VERSION 3.16)
project(bMindMap LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(BM_ENABLE_TESTS "Enable bMindMap tests" ON)

include(FetchContent)

set(BM_GLFW_TARGET "")

find_package(glfw3 CONFIG QUIET)
if(TARGET glfw)
  set(BM_GLFW_TARGET glfw)
elseif(TARGET glfw::glfw)
  set(BM_GLFW_TARGET glfw::glfw)
else()
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
  set(BM_GLFW_TARGET glfw)
endif()

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.9
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_DIR "${imgui_SOURCE_DIR}")

function(bm_target_use_src_includes target_name)
  target_include_directories(${target_name} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
  )
endfunction()

function(bm_target_use_imgui_includes target_name)
  target_include_directories(${target_name} PRIVATE
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
  )
endfunction()

function(bm_link_opengl_for_platform target_name)
  if(WIN32)
    target_link_libraries(${target_name} PRIVATE opengl32)
  elseif(APPLE)
    target_link_libraries(${target_name} PRIVATE
      "-framework OpenGL"
    )
  elseif(UNIX)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${target_name} PRIVATE OpenGL::GL)
  endif()
endfunction()

function(bm_enable_msvc_secure_crt target_name)
  if(MSVC)
    target_compile_definitions(${target_name} PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()
endfunction()

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
  src/ui/MindMapCanvasView.cpp
  src/ui/branch/DrawBranchesBezier.cpp
  src/ui/branch/DrawBranchesOrthogonal.cpp
  src/ui/branch/DrawBranchesOrganicTaper.cpp
)

set(BM_CORE_SOURCES
  src/core/mindmap/SampleMindMapTopology.cpp
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

add_library(bmindmap_core STATIC
  ${BM_CORE_SOURCES}
)

add_library(bmindmap_platform STATIC
  ${BM_PLATFORM_SOURCES}
)

add_library(bmindmap_ui STATIC
  ${BM_UI_SOURCES}
  ${IMGUI_SOURCES}
)

bm_target_use_src_includes(bmindmap_core)
bm_target_use_src_includes(bmindmap_platform)
bm_target_use_src_includes(bmindmap_ui)
bm_target_use_imgui_includes(bmindmap_ui)

target_link_libraries(bmindmap_ui PRIVATE ${BM_GLFW_TARGET})
target_link_libraries(bmindmap_ui PRIVATE bmindmap_core)

target_link_libraries(bmindmap_platform PRIVATE ${BM_GLFW_TARGET})
bm_link_opengl_for_platform(bmindmap_platform)

add_executable(bmindmap
  src/main.cpp
  src/app/AppMain.cpp
)

bm_target_use_src_includes(bmindmap)
bm_target_use_imgui_includes(bmindmap)

target_link_libraries(bmindmap PRIVATE
  bmindmap_platform
  bmindmap_ui
  ${BM_GLFW_TARGET}
)

bm_enable_msvc_secure_crt(bmindmap_platform)
bm_enable_msvc_secure_crt(bmindmap)

if(BM_ENABLE_TESTS)
  enable_testing()

  add_executable(bmindmap_smoke_test
    tests/SmokeTest.cpp
  )

  bm_target_use_src_includes(bmindmap_smoke_test)

  target_link_libraries(bmindmap_smoke_test PRIVATE
    bmindmap_core
  )

  add_test(
    NAME bmindmap_smoke_test
    COMMAND bmindmap_smoke_test
  )
endif()
