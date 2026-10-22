# Keyboard Simulator: the SDL application (window, rendering, input, sound).
add_executable(keyboard-simulator WIN32
  src/main.cpp
  src/options.cpp
  src/draw_util.cpp
  src/palette.cpp
  src/fonts.cpp
  src/settings.cpp
  src/sounds.cpp
  src/text_view.cpp
  src/stats_bar.cpp
  src/keyboard_view.cpp
  src/side_panel.cpp
  src/modal.cpp
  src/dialogs.cpp
  src/trainer_scene.cpp
  res/app.rc)
set_target_properties(keyboard-simulator PROPERTIES OUTPUT_NAME keyboard_simulator)
target_link_libraries(keyboard-simulator PRIVATE vs_engine kbsim_core)
vs_set_output_folder(keyboard-simulator KeyboardSimulator)
vs_add_game_data(keyboard-simulator ${CMAKE_SOURCE_DIR}/keyboard-simulator/data)
