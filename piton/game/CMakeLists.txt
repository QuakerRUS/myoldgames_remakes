# Piton — the game executable (scenes, rendering, audio, saves) on top of vs_engine
# and the SDL-free rules in piton_core.
file(GLOB PITON_GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB PITON_GAME_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp)

add_executable(piton WIN32 ${PITON_GAME_SOURCES} ${PITON_GAME_HEADERS} res/app.rc)
set_target_properties(piton PROPERTIES OUTPUT_NAME piton)
target_link_libraries(piton PRIVATE vs_engine piton_core nlohmann_json::nlohmann_json)

# bin/<Config>/Piton/: piton.exe + SDL3.dll + data/. The company intro uses the shared
# branding art, copied into data/branding (during development also found through VS_BRANDING_DIR).
vs_set_output_folder(piton Piton)
vs_add_game_data(piton ${CMAKE_SOURCE_DIR}/piton/data EXTRA ${CMAKE_SOURCE_DIR}/shared/branding branding)
# Not in Release: a shipped exe must not contain (or look into) the build machine's source path.
target_compile_definitions(piton PRIVATE $<$<NOT:$<CONFIG:Release>>:VS_BRANDING_DIR="${CMAKE_SOURCE_DIR}/shared/branding">)

# SDL-free parts of the game (save signing) with their unit tests.
if(COMMAND vs_add_test)
  vs_add_test(piton_game_tests tests/test_main.cpp tests/test_save.cpp src/hmac_sha256.cpp src/save_codec.cpp)
  target_include_directories(piton_game_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(piton_game_tests PRIVATE piton_core nlohmann_json::nlohmann_json)
endif()
