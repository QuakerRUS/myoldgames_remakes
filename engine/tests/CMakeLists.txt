vs_add_test(engine_tests
  test_main.cpp
  test_version.cpp
  test_utf8.cpp
  test_text_layout.cpp
  test_camera.cpp
  test_sprite_sheet_data.cpp
  test_animation.cpp
  test_undo_stack.cpp
  test_json_file.cpp
  test_storage.cpp
  test_image.cpp
  test_mixer.cpp
  test_text_field.cpp)
target_link_libraries(engine_tests PRIVATE vs_engine)
target_include_directories(engine_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
