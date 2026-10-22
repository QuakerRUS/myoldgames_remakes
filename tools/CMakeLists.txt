# Tools: each subfolder with a CMakeLists.txt is optional (built only outside VS_LOGIC_ONLY).
# tools/import and tools/assets are Python and have no CMake part.
if(NOT VS_LOGIC_ONLY)
  foreach(tool engine-demo painter)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${tool}/CMakeLists.txt)
      add_subdirectory(${tool})
    endif()
  endforeach()
endif()
