# Packaging: `cmake --install build/vs2022 --config Release --prefix dist` produces one
# self-contained folder per program (see tools/package.ps1 for the zips):
#
#   dist/Soldier 2.0/          soldier.exe, soldier_editor.exe, SDL3.dll, data/, README.txt
#   dist/Piton/                piton.exe, piton_editor.exe, SDL3.dll, data/, README.txt
#   dist/Keyboard Simulator/   keyboard_simulator.exe, SDL3.dll, data/, README.txt
#   dist/Painter/              painter.exe, SDL3.dll, README.txt
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

vs_install_folder(Soldier "Soldier 2.0" ${CMAKE_SOURCE_DIR}/soldier/package/README.txt)
vs_install_folder(Piton "Piton" ${CMAKE_SOURCE_DIR}/piton/package/README.txt)
vs_install_folder(KeyboardSimulator "Keyboard Simulator" ${CMAKE_SOURCE_DIR}/keyboard-simulator/package/README.txt)
vs_install_folder(Painter "Painter" ${CMAKE_SOURCE_DIR}/tools/painter/package/README.txt)
