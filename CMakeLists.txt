cmake_minimum_required(VERSION 3.16)
project(MindMapHelper LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(BM_ENABLE_TESTS "Enable MindMap Helper tests" ON)

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
  # Wayland requires wayland-scanner and extra dev packages; X11 is enough for CI and typical dev.
  # (GLFW enables Wayland on Linux by default when dependencies are partially detected.)
  set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)

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

set(FT_DISABLE_ZLIB     ON  CACHE BOOL "" FORCE)  # avoid collision with the in-tree zlib sentinel
set(FT_DISABLE_BZIP2    ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG      ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI   ON  CACHE BOOL "" FORCE)
FetchContent_Declare(
  freetype
  GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
  GIT_TAG        VER-2-13-3
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(freetype)

set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)

# Zlib: normally fetched via git during configure. On locked-down networks you can
# clone madler/zlib separately (tag v1.3.1) and point CMake at it — no fetch at
# configure time. Power users may instead set -DFETCHCONTENT_SOURCE_DIR_ZLIB=...
set(BM_ZLIB_SOURCE_DIR "" CACHE PATH
  "Optional absolute path to a pre-cloned https://github.com/madler/zlib tree (use tag v1.3.1). Skips FetchContent git download.")

if(BM_ZLIB_SOURCE_DIR)
  if(NOT EXISTS "${BM_ZLIB_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "BM_ZLIB_SOURCE_DIR must point to zlib sources (CMakeLists.txt missing): ${BM_ZLIB_SOURCE_DIR}")
  endif()
  FetchContent_Declare(zlib SOURCE_DIR "${BM_ZLIB_SOURCE_DIR}")
else()
  FetchContent_Declare(
    zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG v1.3.1
    GIT_SHALLOW TRUE
  )
endif()
FetchContent_MakeAvailable(zlib)

# libzip calls find_package(ZLIB), which runs FindZLIB. That module only searches
# system prefixes (and ZLIB_ROOT), not the in-tree zlib from FetchContent. MSVC
# environments often have no system zlib, so ZLIB_INCLUDE_DIR / ZLIB_LIBRARY
# stay empty and configure fails even though target zlibstatic exists. Other
# platforms frequently "succeeded" because FindZLIB picked up a system zlib by
# accident while we still linked zlibstatic via ZLIB::ZLIB below.
# Pre-fill FindZLIB's REQUIRED_VARS; actual compile/link uses ZLIB::ZLIB -> zlibstatic.
if(TARGET zlibstatic)
  set(ZLIB_INCLUDE_DIR "${zlib_SOURCE_DIR}" CACHE PATH "Bundled zlib (FindZLIB for libzip)" FORCE)
  set(ZLIB_LIBRARY "${zlib_SOURCE_DIR}/zlib.h" CACHE FILEPATH "Bundled zlib sentinel for FindZLIB (libzip)" FORCE)
endif()

# Stock FindZLIB (invoked from libzip's subdirectory) creates ZLIB::ZLIB without
# GLOBAL scope, so parent targets cannot link it — CI then fails linking
# mindmap_helper_core. Define a GLOBAL INTERFACE target up front so libzip and
# core both use bundled zlibstatic (CMake skips recreating ZLIB::ZLIB in FindZLIB).
if(TARGET zlibstatic AND NOT TARGET ZLIB::ZLIB)
  add_library(ZLIB::ZLIB INTERFACE IMPORTED GLOBAL)
  get_target_property(_bm_zlib_iface_includes zlibstatic INTERFACE_INCLUDE_DIRECTORIES)
  set_target_properties(ZLIB::ZLIB PROPERTIES
    INTERFACE_LINK_LIBRARIES zlibstatic
    INTERFACE_INCLUDE_DIRECTORIES "${_bm_zlib_iface_includes}"
  )
endif()

set(ENABLE_BZIP2 OFF CACHE BOOL "" FORCE)
set(ENABLE_LZMA OFF CACHE BOOL "" FORCE)
set(ENABLE_ZSTD OFF CACHE BOOL "" FORCE)
set(ENABLE_GNUTLS OFF CACHE BOOL "" FORCE)
set(ENABLE_MBEDTLS OFF CACHE BOOL "" FORCE)
set(ENABLE_OPENSSL OFF CACHE BOOL "" FORCE)
if(APPLE)
  set(ENABLE_COMMONCRYPTO ON CACHE BOOL "" FORCE)
else()
  set(ENABLE_COMMONCRYPTO OFF CACHE BOOL "" FORCE)
endif()
if(WIN32)
  set(ENABLE_WINDOWS_CRYPTO ON CACHE BOOL "" FORCE)
  set(ENABLE_FDOPEN OFF CACHE BOOL "" FORCE)
else()
  set(ENABLE_WINDOWS_CRYPTO OFF CACHE BOOL "" FORCE)
endif()

set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BUILD_REGRESS OFF CACHE BOOL "" FORCE)
set(BUILD_OSSFUZZ OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_DOC OFF CACHE BOOL "" FORCE)
set(LIBZIP_DO_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  libzip
  GIT_REPOSITORY https://github.com/nih-at/libzip.git
  GIT_TAG v1.11.3
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(libzip)

set(PUGIXML_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  pugixml
  GIT_REPOSITORY https://github.com/zeux/pugixml.git
  GIT_TAG v1.15
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(pugixml)

FetchContent_Declare(
  ImGuiFileDialog
  GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog.git
  GIT_TAG v0.6.8
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ImGuiFileDialog)
# We compile ImGuiFileDialog.cpp ourselves via BM_UI_SOURCES (imgui includes come from mindmap_helper_ui).
# Exclude the library target created by ImGuiFileDialog's own CMakeLists.txt; it lacks imgui and is unused.
set_target_properties(ImGuiFileDialog PROPERTIES EXCLUDE_FROM_ALL TRUE)

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
  ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
)

set(BM_UI_SOURCES
  ${imguifiledialog_SOURCE_DIR}/ImGuiFileDialog.cpp
  src/ui/MindMapUi.cpp
  src/ui/MindMapCanvasView.cpp
  src/ui/UiCommandDispatcher.cpp
  src/ui/commands/CollapseNodeCommand.cpp
  src/ui/commands/DeleteNodeCommand.cpp
  src/ui/commands/InsertChildNodeCommand.cpp
  src/ui/branch/DrawBranchesBezier.cpp
  src/ui/branch/DrawBranchesOrthogonal.cpp
  src/ui/branch/DrawBranchesOrganicTaper.cpp
  src/ui/branch/DrawBranchTextOnPath.cpp
  src/ui/canvas/NodeTextureUtils.cpp
  src/ui/canvas/MindMapCanvasTreeNav.cpp
  src/ui/canvas/MindMapCanvasNodeMutators.cpp
  src/ui/commands/PasteImageCommand.cpp
  src/ui/commands/PasteTextCommand.cpp
)

set(BM_CORE_SOURCES
  src/core/imx/ImxMindMapLoader.cpp
  src/core/imx/ImxImportAdapter.cpp
  src/core/ImportService.cpp
  src/core/DocumentPathLoader.cpp
  src/core/mindmap/UuidGenerator.cpp
  src/core/JsonNativeDocumentRepository.cpp
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

add_library(mindmap_helper_core STATIC
  ${BM_CORE_SOURCES}
)

add_library(mindmap_helper_platform STATIC
  ${BM_PLATFORM_SOURCES}
)

add_library(mindmap_helper_ui STATIC
  ${BM_UI_SOURCES}
  ${IMGUI_SOURCES}
)

bm_target_use_src_includes(mindmap_helper_core)
bm_target_use_src_includes(mindmap_helper_platform)
bm_target_use_src_includes(mindmap_helper_ui)
bm_target_use_imgui_includes(mindmap_helper_ui)

target_link_libraries(mindmap_helper_core PRIVATE
  nlohmann_json::nlohmann_json
  libzip::zip
  ZLIB::ZLIB
  pugixml::pugixml
)
target_include_directories(mindmap_helper_ui PRIVATE "${imguifiledialog_SOURCE_DIR}")

target_compile_definitions(mindmap_helper_ui PRIVATE
  IMGUI_USER_CONFIG="ui/imconfig.h"
)
target_include_directories(mindmap_helper_ui PRIVATE
  "${IMGUI_DIR}/misc/freetype"
)
target_link_libraries(mindmap_helper_ui PRIVATE freetype)
target_link_libraries(mindmap_helper_ui PRIVATE ${BM_GLFW_TARGET})
target_link_libraries(mindmap_helper_ui PRIVATE mindmap_helper_core)
bm_link_opengl_for_platform(mindmap_helper_ui)

if(APPLE)
  target_sources(mindmap_helper_ui PRIVATE src/ui/canvas/ClipboardImage_macos.mm)
  target_link_libraries(mindmap_helper_ui PRIVATE "-framework AppKit")
elseif(WIN32)
  target_sources(mindmap_helper_ui PRIVATE src/ui/canvas/ClipboardImage_windows.cpp)
elseif(UNIX)
  target_sources(mindmap_helper_ui PRIVATE src/ui/canvas/ClipboardImage_linux.cpp)
endif()

target_link_libraries(mindmap_helper_platform PRIVATE ${BM_GLFW_TARGET})
bm_link_opengl_for_platform(mindmap_helper_platform)

add_executable(mindmap_helper
  src/main.cpp
  src/app/AppMain.cpp
  src/app/DocumentSessionService.cpp
  src/app/HelpMindMapDocument.cpp
)

set_target_properties(mindmap_helper PROPERTIES OUTPUT_NAME "MindMapHelper")

bm_target_use_src_includes(mindmap_helper)
bm_target_use_imgui_includes(mindmap_helper)

target_link_libraries(mindmap_helper PRIVATE
  mindmap_helper_platform
  mindmap_helper_ui
  ${BM_GLFW_TARGET}
)

bm_enable_msvc_secure_crt(mindmap_helper_platform)
bm_enable_msvc_secure_crt(mindmap_helper)

if(BM_ENABLE_TESTS)
  enable_testing()

  add_executable(mindmap_helper_smoke_test
    tests/SmokeTest.cpp
  )

  bm_target_use_src_includes(mindmap_helper_smoke_test)

  target_link_libraries(mindmap_helper_smoke_test PRIVATE
    mindmap_helper_core
  )

  add_test(
    NAME mindmap_helper_smoke_test
    COMMAND mindmap_helper_smoke_test
  )

  add_executable(mindmap_helper_imx_loader_test
    tests/ImxMindMapLoaderTest.cpp
  )

  bm_target_use_src_includes(mindmap_helper_imx_loader_test)

  target_link_libraries(mindmap_helper_imx_loader_test PRIVATE
    mindmap_helper_core
    libzip::zip
  )

  add_test(
    NAME mindmap_helper_imx_loader_test
    COMMAND mindmap_helper_imx_loader_test
  )

  add_executable(mindmap_helper_uuid_test
    tests/UuidGeneratorTest.cpp
  )
  bm_target_use_src_includes(mindmap_helper_uuid_test)
  target_link_libraries(mindmap_helper_uuid_test PRIVATE mindmap_helper_core)
  add_test(NAME mindmap_helper_uuid_test COMMAND mindmap_helper_uuid_test)

  add_executable(mindmap_helper_json_round_trip_test
    tests/JsonRoundTripTest.cpp
  )
  bm_target_use_src_includes(mindmap_helper_json_round_trip_test)
  target_link_libraries(mindmap_helper_json_round_trip_test PRIVATE mindmap_helper_core)
  add_test(NAME mindmap_helper_json_round_trip_test COMMAND mindmap_helper_json_round_trip_test)

  add_executable(mindmap_helper_imx_import_adapter_test
    tests/ImxImportAdapterTest.cpp
  )
  bm_target_use_src_includes(mindmap_helper_imx_import_adapter_test)
  target_link_libraries(mindmap_helper_imx_import_adapter_test PRIVATE mindmap_helper_core)
  add_test(NAME mindmap_helper_imx_import_adapter_test COMMAND mindmap_helper_imx_import_adapter_test)

  add_executable(mindmap_helper_import_service_test
    tests/ImportServiceTest.cpp
  )
  bm_target_use_src_includes(mindmap_helper_import_service_test)
  target_link_libraries(mindmap_helper_import_service_test PRIVATE mindmap_helper_core)
  add_test(NAME mindmap_helper_import_service_test COMMAND mindmap_helper_import_service_test)

  add_executable(mindmap_helper_document_path_loader_test
    tests/DocumentPathLoaderTest.cpp
  )
  bm_target_use_src_includes(mindmap_helper_document_path_loader_test)
  target_link_libraries(mindmap_helper_document_path_loader_test PRIVATE mindmap_helper_core libzip::zip)
  add_test(NAME mindmap_helper_document_path_loader_test COMMAND mindmap_helper_document_path_loader_test)

  add_executable(mindmap_helper_document_session_path_test
    tests/DocumentSessionServicePathTest.cpp
    src/app/DocumentSessionService.cpp
  )
  bm_target_use_src_includes(mindmap_helper_document_session_path_test)
  target_link_libraries(mindmap_helper_document_session_path_test PRIVATE mindmap_helper_core libzip::zip)
  add_test(NAME mindmap_helper_document_session_path_test COMMAND mindmap_helper_document_session_path_test)

  add_executable(mindmap_helper_command_history_test
    tests/CommandHistoryTest.cpp
  )
  bm_target_use_src_includes(mindmap_helper_command_history_test)
  add_test(NAME mindmap_helper_command_history_test COMMAND mindmap_helper_command_history_test)

  add_executable(mindmap_helper_base64_test
    tests/Base64Test.cpp
  )
  bm_target_use_src_includes(mindmap_helper_base64_test)
  add_test(NAME mindmap_helper_base64_test COMMAND mindmap_helper_base64_test)

  add_executable(mindmap_helper_help_doc_test
    tests/HelpMindMapDocumentTest.cpp
    src/app/HelpMindMapDocument.cpp
  )
  bm_target_use_src_includes(mindmap_helper_help_doc_test)
  target_link_libraries(mindmap_helper_help_doc_test PRIVATE mindmap_helper_core)
  add_test(NAME mindmap_helper_help_doc_test COMMAND mindmap_helper_help_doc_test)

endif()
