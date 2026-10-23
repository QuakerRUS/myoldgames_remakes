# Packaging: `cmake --install build/vs2022 --config Release --prefix dist` produces one
# self-contained folder per game, named "<game> <version>" (see tools/package.ps1 for the zips):
#
#   dist/Soldier 2.0/              soldier.exe, soldier_editor.exe, painter.exe, SDL3.dll, data/,
#                                  README.txt, PAINTER.txt
#   dist/Piton 1.0/                piton.exe, piton_editor.exe, painter.exe, SDL3.dll, data/,
#                                  README.txt, PAINTER.txt
#   dist/Keyboard Simulator 1.0/   keyboard_simulator.exe, SDL3.dll, data/, README.txt
#
# Painter, the graphics editor shared by Soldier and Piton, ships with both games.
#
# Each folder also gets the Visual C++ runtime DLLs (app-local deployment), so the programs run
# on a Windows PC without the Visual C++ Redistributable.

set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
set(CMAKE_INSTALL_UCRT_LIBRARIES FALSE)
include(InstallRequiredSystemLibraries)

# vs_install_folder(<output folder> <package folder> <readme.txt>): installs every program built
# into bin/<Config>/<output folder> (see vs_set_output_folder), SDL3.dll, the folder's data
# (vs_add_game_data, including its EXTRA directories) and the README into <prefix>/<package folder>.
function(vs_install_folder folder package readme)
  get_property(targets GLOBAL PROPERTY VS_FOLDER_TARGETS_${folder})
  if(NOT targets)
    return()
  endif()
  install(TARGETS ${targets} RUNTIME DESTINATION "${package}")
  install(FILES $<TARGET_FILE:SDL3::SDL3> DESTINATION "${package}")
  if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
    install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION "${package}")
  endif()
  get_property(data GLOBAL PROPERTY VS_FOLDER_DATA_${folder})
  if(data)
    install(DIRECTORY "${data}/" DESTINATION "${package}/data")
  endif()
  get_property(extras GLOBAL PROPERTY VS_FOLDER_EXTRA_DIRS_${folder})
  foreach(entry IN LISTS extras)
    string(REPLACE "|" ";" pair "${entry}")
    list(GET pair 0 src)
    list(GET pair 1 sub)
    install(DIRECTORY "${src}/" DESTINATION "${package}/data/${sub}")
  endforeach()
  install(FILES "${readme}" DESTINATION "${package}" RENAME README.txt)
endfunction()

# vs_install_tools(<output folder> <package folder> <readme.txt> <readme name>): adds the programs
# of another output folder (no data of their own) and their README to a package folder that
# vs_install_folder already filled.
function(vs_install_tools folder package readme readme_name)
  get_property(targets GLOBAL PROPERTY VS_FOLDER_TARGETS_${folder})
  if(NOT targets)
    return()
  endif()
  install(TARGETS ${targets} RUNTIME DESTINATION "${package}")
  install(FILES "${readme}" DESTINATION "${package}" RENAME "${readme_name}")
endfunction()

set(VS_SOLDIER_PACKAGE "Soldier 2.0")
set(VS_PITON_PACKAGE "Piton 1.0")
set(VS_KBSIM_PACKAGE "Keyboard Simulator 1.0")

vs_install_folder(Soldier "${VS_SOLDIER_PACKAGE}" ${CMAKE_SOURCE_DIR}/soldier/package/README.txt)
vs_install_folder(Piton "${VS_PITON_PACKAGE}" ${CMAKE_SOURCE_DIR}/piton/package/README.txt)
vs_install_folder(KeyboardSimulator "${VS_KBSIM_PACKAGE}" ${CMAKE_SOURCE_DIR}/keyboard-simulator/package/README.txt)
vs_install_tools(Painter "${VS_SOLDIER_PACKAGE}" ${CMAKE_SOURCE_DIR}/tools/painter/package/README.txt PAINTER.txt)
vs_install_tools(Painter "${VS_PITON_PACKAGE}" ${CMAKE_SOURCE_DIR}/tools/painter/package/README.txt PAINTER.txt)
