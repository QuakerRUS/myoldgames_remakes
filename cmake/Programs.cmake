# Output folders, game data and runtime DLLs of the programs.
#
# Every program lives in its own folder next to its data, so that programs with equally named
# data files (Soldier and Piton both have gfx/tiles.json) do not overwrite each other:
#
#   build/<preset>/bin/<Config>/Soldier/            soldier.exe, soldier_editor.exe, SDL3.dll, data/
#   build/<preset>/bin/<Config>/Piton/              piton.exe, piton_editor.exe, SDL3.dll, data/
#   build/<preset>/bin/<Config>/KeyboardSimulator/  keyboard_simulator.exe, SDL3.dll, data/
#   build/<preset>/bin/<Config>/Painter/            painter.exe, SDL3.dll
#   build/<preset>/bin/<Config>/                    tests, engine-demo, SDL3.dll
#
# Usage (the order matters: the folder first):
#   vs_set_output_folder(soldier Soldier)
#   vs_add_game_data(soldier ${CMAKE_SOURCE_DIR}/soldier/data
#                    EXTRA ${CMAKE_SOURCE_DIR}/shared/branding branding)
#   vs_set_output_folder(soldier-editor Soldier)
#   vs_add_game_data(soldier-editor ${CMAKE_SOURCE_DIR}/soldier/data)
#
# The copies are made by one custom target per folder (vs-runtime-<folder>, vs-data-<folder>),
# so two programs of one folder never write the same files at the same time in a parallel build.
# cmake/Install.cmake packages the folders for `cmake --install`.

# vs_set_output_folder(<target> <folder>): the target's exe goes to bin/<Config>/<folder>/ and
# SDL3.dll is copied there.
function(vs_set_output_folder target folder)
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${folder}"
    VS_OUTPUT_FOLDER "${folder}")
  set_property(GLOBAL APPEND PROPERTY VS_FOLDER_TARGETS_${folder} ${target})
  get_property(folders GLOBAL PROPERTY VS_OUTPUT_FOLDERS)
  if(NOT folder IN_LIST folders)
    set_property(GLOBAL APPEND PROPERTY VS_OUTPUT_FOLDERS ${folder})
  endif()
  vs_copy_runtime(${target})
endfunction()

# vs_copy_runtime(<target>): makes sure SDL3.dll is next to the target's executable. Targets in
# the default output directory (bin/<Config>/) already have it there.
function(vs_copy_runtime target)
  get_target_property(folder ${target} VS_OUTPUT_FOLDER)
  if(NOT folder)
    return()
  endif()
  set(copy_target vs-runtime-${folder})
  if(NOT TARGET ${copy_target})
    add_custom_target(${copy_target}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${folder}"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL3::SDL3>
              "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${folder}/"
      COMMENT "SDL3.dll -> bin/<Config>/${folder}"
      VERBATIM)
    get_target_property(sdl SDL3::SDL3 ALIASED_TARGET)
    if(sdl)
      add_dependencies(${copy_target} ${sdl})
    endif()
    set_target_properties(${copy_target} PROPERTIES FOLDER "copy")
  endif()
  add_dependencies(${target} ${copy_target})
endfunction()

set(VS_SYNC_DATA_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/SyncData.cmake")

# vs_add_game_data(<target> <data_dir> [EXTRA <src_dir> <data_subdir>]...): mirrors <data_dir>
# to bin/<Config>/<folder>/data (and every EXTRA <src_dir> to data/<data_subdir>) before the
# target is built (cmake/SyncData.cmake: changed files are copied, files deleted or renamed in
# the sources are removed from the copy), and defines VS_DEV_DATA_DIR for it: the absolute
# <data_dir> in Debug/RelWithDebInfo/MinSizeRel, "" in Release (a shipped exe must not embed the
# developer's source path; it finds everything in <exe_dir>/data).
# vs::init_asset_roots(VS_DEV_DATA_DIR) searches <exe_dir>/data first and the source tree as a
# development fallback. The first call for a folder defines its data; later calls (another
# program of the same folder) must name the same <data_dir> and may omit EXTRA.
# Do not keep hand-made files in bin/<Config>/<folder>/data: the next build removes them.
function(vs_add_game_data target data_dir)
  cmake_parse_arguments(ARG "" "" "EXTRA" ${ARGN})
  get_filename_component(abs "${data_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  get_target_property(folder ${target} VS_OUTPUT_FOLDER)
  if(NOT folder)
    message(FATAL_ERROR "vs_add_game_data(${target}): call vs_set_output_folder(${target} <folder>) first")
  endif()
  target_compile_definitions(${target} PRIVATE "VS_DEV_DATA_DIR=\"$<$<NOT:$<CONFIG:Release>>:${abs}>\"")

  set(copy_target vs-data-${folder})
  if(TARGET ${copy_target})
    get_property(known GLOBAL PROPERTY VS_FOLDER_DATA_${folder})
    get_property(known_extra GLOBAL PROPERTY VS_FOLDER_EXTRA_${folder})
    if(NOT known STREQUAL abs OR (ARG_EXTRA AND NOT ARG_EXTRA STREQUAL known_extra))
      message(FATAL_ERROR "vs_add_game_data(${target}): folder ${folder} already has other data (${known})")
    endif()
  else()
    set(dest "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${folder}/data")
    set(sources "${abs}|.")
    set(extra ${ARG_EXTRA})
    list(LENGTH extra n)
    math(EXPR odd "${n} % 2")
    if(odd)
      message(FATAL_ERROR "vs_add_game_data(${target}): EXTRA takes pairs <src_dir> <data_subdir>")
    endif()
    while(extra)
      list(POP_FRONT extra src sub)
      get_filename_component(src "${src}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
      list(APPEND sources "${src}|${sub}")
      set_property(GLOBAL APPEND PROPERTY VS_FOLDER_EXTRA_DIRS_${folder} "${src}|${sub}")
    endwhile()
    set(config_file "${CMAKE_BINARY_DIR}/vs-data/${folder}.cmake")
    file(CONFIGURE OUTPUT "${config_file}" CONTENT "set(VS_SYNC_SOURCES [==[@sources@]==])
" @ONLY)
    add_custom_target(${copy_target}
      COMMAND ${CMAKE_COMMAND} "-DVS_SYNC_CONFIG=${config_file}" "-DVS_SYNC_DEST=${dest}" -P "${VS_SYNC_DATA_SCRIPT}"
      COMMENT "data -> bin/<Config>/${folder}/data"
      VERBATIM)
    set_target_properties(${copy_target} PROPERTIES FOLDER "copy")
    set_property(GLOBAL PROPERTY VS_FOLDER_DATA_${folder} "${abs}")
    set_property(GLOBAL PROPERTY VS_FOLDER_EXTRA_${folder} "${ARG_EXTRA}")
  endif()
  add_dependencies(${target} ${copy_target})
endfunction()
