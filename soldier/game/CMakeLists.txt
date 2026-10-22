# Soldier 2.0 — the game executable (scenes, rendering, audio, saves) on top of vs_engine
# and the SDL-free rules in soldier_core.
file(GLOB SOLDIER_GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB SOLDIER_GAME_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp)

add_executable(soldier WIN32 ${SOLDIER_GAME_SOURCES} ${SOLDIER_GAME_HEADERS} res/app.rc)
set_target_properties(soldier PROPERTIES OUTPUT_NAME soldier)
target_link_libraries(soldier PRIVATE vs_engine soldier_core nlohmann_json::nlohmann_json)

# bin/<Config>/Soldier/: soldier.exe + SDL3.dll + data/. The company intro uses the shared
# branding art, copied into data/branding (during development also found through VS_BRANDING_DIR).
vs_set_output_folder(soldier Soldier)
vs_add_game_data(soldier ${CMAKE_SOURCE_DIR}/soldier/data EXTRA ${CMAKE_SOURCE_DIR}/shared/branding branding)
# Development fallback only: a Release exe must not contain source-tree paths.
target_compile_definitions(soldier PRIVATE $<$<NOT:$<CONFIG:Release>>:VS_BRANDING_DIR="${CMAKE_SOURCE_DIR}/shared/branding">)
