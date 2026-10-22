# Painter: the shared graphics editor (PNG images and engine sprite sheets).
#   painter [file]  |  --roundtrip <dir> <outdir>  |  --screenshot <png> --open <file>  |  --smoke

# SDL-free core: document, sheet JSON, raster algorithms, palettes, undo commands, round-trip check.
add_library(painter_core STATIC
  src/document.cpp
  src/sheet_json.cpp
  src/sheet_ops.cpp
  src/raster.cpp
  src/palette.cpp
  src/commands.cpp
  src/roundtrip.cpp)
target_include_directories(painter_core PUBLIC src)
target_link_libraries(painter_core PUBLIC vs_engine)

add_executable(painter WIN32
  src/main.cpp
  src/painter_scene.cpp
  src/painter.cpp
  src/painter_file.cpp
  src/painter_edit.cpp
  src/painter_selection.cpp
  src/painter_tools.cpp
  src/painter_canvas.cpp
  src/painter_keys.cpp
  src/painter_menu.cpp
  src/painter_frames.cpp
  src/painter_animations.cpp
  src/painter_colors.cpp
  src/painter_dialogs.cpp
  src/painter_selftest.cpp
  src/tools.cpp
  src/ui_util.cpp
  res/app.rc)
target_link_libraries(painter PRIVATE painter_core vs_editor vs_imgui vs_engine)
vs_set_output_folder(painter Painter)

if(BUILD_TESTING)
  vs_add_test(painter_tests
    tests/test_main.cpp
    tests/test_raster.cpp
    tests/test_palette.cpp
    tests/test_sheet.cpp
    tests/test_document.cpp)
  target_link_libraries(painter_tests PRIVATE painter_core)
endif()
