# Soldier 2.0 Map Editor (soldier_editor.exe) on vs_editor + soldier_core.
set(SOLDIER_EDITOR_LOGIC
  src/palette.cpp
  src/level_io.cpp
  src/level_check.cpp
  src/placement.cpp)

add_executable(soldier-editor WIN32
  src/main.cpp
  src/soldier_plugin.cpp
  src/tile_painter.cpp
  ${SOLDIER_EDITOR_LOGIC}
  res/app.rc)
set_target_properties(soldier-editor PROPERTIES OUTPUT_NAME soldier_editor)
target_link_libraries(soldier-editor PRIVATE vs_editor soldier_core nlohmann_json::nlohmann_json)
# Next to soldier.exe (F5 starts it from the editor's folder).
vs_set_output_folder(soldier-editor Soldier)
vs_add_game_data(soldier-editor ${CMAKE_SOURCE_DIR}/soldier/data)

if(BUILD_TESTING)
  # SDL-free editor logic: level I/O round trip, validation, placement, palette navigation.
  vs_add_test(soldier_editor_tests
    tests/main.cpp
    tests/editor_logic_test.cpp
    ${SOLDIER_EDITOR_LOGIC})
  target_include_directories(soldier_editor_tests PRIVATE src)
  target_link_libraries(soldier_editor_tests PRIVATE vs_editor soldier_core nlohmann_json::nlohmann_json)
  target_compile_definitions(soldier_editor_tests PRIVATE SOLDIER_DATA_DIR="${CMAKE_SOURCE_DIR}/soldier/data")
endif()
