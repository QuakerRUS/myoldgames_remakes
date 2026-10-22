cmake_minimum_required(VERSION 3.28)
project(remakes VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Tests and tools land in bin/<Config>/; the programs in bin/<Config>/<folder>/ (cmake/Programs.cmake).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8 /MP)
endif()

include(CTest)

# VS_LOGIC_ONLY builds only the SDL-free logic libraries and their tests (fast; no SDL fetch).
option(VS_LOGIC_ONLY "Build only logic libraries and tests" OFF)

include(cmake/Dependencies.cmake)
include(cmake/Programs.cmake)

# vs_add_test(<name> <sources...>): doctest executable registered with CTest.
# The caller links the library under test: target_link_libraries(<name> PRIVATE <lib>).
function(vs_add_test name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE doctest::doctest)
  add_test(NAME ${name} COMMAND ${name} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endfunction()

if(NOT VS_LOGIC_ONLY)
  add_subdirectory(engine)
endif()
foreach(dir soldier piton keyboard-simulator tools)
  if(EXISTS ${CMAKE_SOURCE_DIR}/${dir}/CMakeLists.txt)
    add_subdirectory(${dir})
  endif()
endforeach()

if(NOT VS_LOGIC_ONLY)
  include(cmake/Install.cmake)
endif()
