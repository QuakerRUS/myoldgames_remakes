include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# Every dependency is pinned to an exact tag or commit (verified with `git ls-remote`).
# Archives downloaded by URL are pinned by SHA-256 as well.

set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.4.18 # 829a65d769d935c4852f8159e964312c0957260a
  GIT_SHALLOW TRUE)

# Only our programs are installed (cmake/Install.cmake), not the test framework's headers.
set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.5.3 # 2d0a9359a60c51affe2a9bebb1be1dca47868151
  GIT_SHALLOW TRUE)

# nlohmann/json v3.12.0 (header-only). Available in every mode, including VS_LOGIC_ONLY,
# as target nlohmann_json::nlohmann_json.
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
FetchContent_Declare(nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
  URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

# stb (stb_image, stb_image_write, stb_truetype, stb_vorbis) at master commit a7d4c217.
FetchContent_Declare(stb
  URL https://github.com/nothings/stb/archive/a7d4c2175630e1ea5b6f8dd00ab96fed0b97b96d.zip
  URL_HASH SHA256=fc6e7c840a0e025ee238fc09e136452614a4b765afa6ff4177d2e9e1b3493675
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

# Dear ImGui v1.93.0 (c1aaac65e896a45f81f47a4d68f9959e0f86c859).
FetchContent_Declare(imgui
  URL https://github.com/ocornut/imgui/archive/refs/tags/v1.93.0.tar.gz
  URL_HASH SHA256=f7bf6bc76e5d79d40d751096663e6c5d18daf08129dd52e7f7b09e4251c8eeee
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

if(VS_LOGIC_ONLY)
  FetchContent_MakeAvailable(doctest nlohmann_json)
else()
  # stb and imgui have no CMakeLists.txt at their roots: MakeAvailable only downloads them;
  # engine/CMakeLists.txt builds the targets from ${stb_SOURCE_DIR} and ${imgui_SOURCE_DIR}.
  FetchContent_MakeAvailable(SDL3 doctest nlohmann_json stb imgui)
endif()

# doctest's own forward declarations of std types break MSVC's <string_view> comparisons
# (CHECK(sv == "...") fails to compile); use the real standard headers instead.
target_compile_definitions(doctest INTERFACE DOCTEST_CONFIG_USE_STD_HEADERS)
