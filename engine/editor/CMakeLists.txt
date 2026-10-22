add_library(vs_editor STATIC
  src/tile_document.cpp
  src/palette_grid.cpp
  src/grid_view.cpp
  src/file_dialogs.cpp
  src/confirm_prompt.cpp
  src/save_flow.cpp
  src/tile_editor.cpp
  src/run_tile_editor.cpp)
target_include_directories(vs_editor PUBLIC include)
target_link_libraries(vs_editor PUBLIC vs_imgui vs_engine)

if(BUILD_TESTING)
  vs_add_test(editor_tests ../tests/test_main.cpp tests/test_tile_document.cpp tests/test_palette_grid.cpp tests/test_save_flow.cpp)
  target_link_libraries(editor_tests PRIVATE vs_editor)
endif()
