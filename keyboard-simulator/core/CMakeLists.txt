# kbsim_core: SDL-free logic of Keyboard Simulator (texts, layout, typing session).
add_library(kbsim_core STATIC
  src/utf8.cpp
  src/json.cpp
  src/text_source.cpp
  src/layout.cpp
  src/session.cpp)
target_include_directories(kbsim_core PUBLIC include PRIVATE src)
target_compile_features(kbsim_core PUBLIC cxx_std_20)

if(BUILD_TESTING)
  vs_add_test(kbsim_core_tests
    tests/test_main.cpp
    tests/test_utf8.cpp
    tests/test_text_source.cpp
    tests/test_layout.cpp
    tests/test_session.cpp)
  target_link_libraries(kbsim_core_tests PRIVATE kbsim_core)
  target_include_directories(kbsim_core_tests PRIVATE src)
  # The real data files are tested too (every sentence must be typeable on the layout).
  target_compile_definitions(kbsim_core_tests PRIVATE
    KBSIM_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data")
endif()
