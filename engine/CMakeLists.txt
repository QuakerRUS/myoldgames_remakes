# --- third-party: stb (implementation unit built without our warning level) ---
add_library(vs_stb STATIC src/stb_impl.cpp)
target_include_directories(vs_stb SYSTEM PUBLIC ${stb_SOURCE_DIR})
if(MSVC)
  set_target_properties(vs_stb PROPERTIES COMPILE_OPTIONS "/W0;/utf-8;/MP")
else()
  set_target_properties(vs_stb PROPERTIES COMPILE_OPTIONS "-w")
endif()

# --- vs_engine: the shared game engine (namespace vs) ---
add_library(vs_engine STATIC)
target_include_directories(vs_engine PUBLIC include)
target_link_libraries(vs_engine PUBLIC SDL3::SDL3 nlohmann_json::nlohmann_json PRIVATE vs_stb)
target_compile_features(vs_engine PUBLIC cxx_std_20)
target_sources(vs_engine PRIVATE
  src/version.cpp
  # SDL-free
  src/utf8.cpp
  src/text_layout.cpp
  src/camera.cpp
  src/animation.cpp
  src/undo_stack.cpp
  src/json_file.cpp
  src/sprite_sheet_data.cpp
  src/image.cpp
  src/mixer.cpp
  # SDL
  src/app.cpp
  src/renderer.cpp
  src/texture.cpp
  src/sprite_sheet.cpp
  src/font.cpp
  src/audio.cpp
  src/input.cpp
  src/storage.cpp
  src/screenshot.cpp
  src/window_icon.cpp
  src/console.cpp
  src/ui_menu.cpp
  src/ui_text_field.cpp
  src/ui_dialog.cpp
  src/ui_button.cpp)

# --- vs_imgui: Dear ImGui + SDL3 / SDL_Renderer backends, and vs::ImGuiLayer ---
add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp)
target_include_directories(imgui SYSTEM PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends ${imgui_SOURCE_DIR}/misc/cpp)
target_link_libraries(imgui PUBLIC SDL3::SDL3)
if(MSVC)
  set_target_properties(imgui PROPERTIES COMPILE_OPTIONS "/W0;/utf-8;/MP")
else()
  set_target_properties(imgui PROPERTIES COMPILE_OPTIONS "-w")
endif()

add_library(vs_imgui STATIC imgui/src/imgui_layer.cpp)
target_include_directories(vs_imgui PUBLIC imgui/include)
target_link_libraries(vs_imgui PUBLIC imgui vs_engine)

# --- vs_editor: base of the tile map editors ---
add_subdirectory(editor)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
