add_library(soldier_core STATIC
  src/tile.cpp
  src/names.cpp
  src/level.cpp
  src/game.cpp
  src/recording.cpp
  src/progress.cpp)
target_include_directories(soldier_core PUBLIC include PRIVATE src)
target_compile_features(soldier_core PUBLIC cxx_std_20)
target_link_libraries(soldier_core PRIVATE nlohmann_json::nlohmann_json)

if(BUILD_TESTING)
  vs_add_test(soldier_core_tests
    tests/main.cpp
    tests/tiles_test.cpp
    tests/level_test.cpp
    tests/rules_test.cpp
    tests/keys_test.cpp
    tests/timing_test.cpp
    tests/recording_test.cpp
    tests/records_test.cpp
    tests/progress_test.cpp)
  target_link_libraries(soldier_core_tests PRIVATE soldier_core)
  target_compile_definitions(soldier_core_tests PRIVATE
    SOLDIER_DATA_DIR="${CMAKE_SOURCE_DIR}/soldier/data")
endif()
