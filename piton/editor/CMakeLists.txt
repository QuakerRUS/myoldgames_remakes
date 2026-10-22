# Piton Map Editor (piton_editor.exe) on vs_editor + piton_core.
add_executable(piton-editor WIN32
  src/main.cpp
  src/level_doc.cpp
  src/level_doc.hpp
  src/piton_editor.cpp
  src/piton_editor.hpp
  res/app.rc)
set_target_properties(piton-editor PROPERTIES OUTPUT_NAME piton_editor)
target_link_libraries(piton-editor PRIVATE vs_editor piton_core)
# Next to piton.exe (F5 starts it from the editor's folder).
vs_set_output_folder(piton-editor Piton)
vs_add_game_data(piton-editor ${CMAKE_SOURCE_DIR}/piton/data)

if(BUILD_TESTING)
  # Document model, tools and file round trip (SDL-free part).
  vs_add_test(piton_editor_tests tests/test_main.cpp tests/test_level_doc.cpp src/level_doc.cpp)
  target_include_directories(piton_editor_tests PRIVATE src)
  target_link_libraries(piton_editor_tests PRIVATE vs_editor piton_core)
  target_compile_definitions(piton_editor_tests PRIVATE PITON_LEVELS_DIR="${CMAKE_SOURCE_DIR}/piton/data/levels")
  # The exe's own self-check over the shipped levels (byte-identical load/save).
  add_test(NAME piton_editor_roundtrip
           COMMAND piton-editor --roundtrip ${CMAKE_SOURCE_DIR}/piton/data/levels)
endif()
