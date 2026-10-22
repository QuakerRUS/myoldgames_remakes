# Each part is optional so that parts can be developed in parallel.
foreach(part core app)
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${part}/CMakeLists.txt)
    if(part STREQUAL "core" OR NOT VS_LOGIC_ONLY)
      add_subdirectory(${part})
    endif()
  endif()
endforeach()
