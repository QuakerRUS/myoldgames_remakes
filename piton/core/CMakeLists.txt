# piton_core: SDL-free Piton rules, levels and the demo AI.
add_library(piton_core STATIC
  src/level.cpp
  src/game.cpp
  src/demo_pilot.cpp
)
target_include_directories(piton_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(piton_core PUBLIC cxx_std_20)

if(COMMAND vs_add_test)
  vs_add_test(piton_core_tests
    tests/test_main.cpp
    tests/test_rules.cpp
    tests/test_levels.cpp
    tests/test_pilot.cpp
  )
  target_link_libraries(piton_core_tests PRIVATE piton_core)
  target_compile_definitions(piton_core_tests PRIVATE
    PITON_LEVELS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../data/levels")
endif()
