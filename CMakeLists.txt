cmake_minimum_required(VERSION 3.15)

project(Castr
  VERSION 1.0.0
  DESCRIPTION "Castr"
  LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if (MSVC)
  add_compile_options(/W4 /02)

  add_compile_options(/Wall)

  # add_compile_options(/WX)

  message(STATUS "Configuring for msvc compiler")
endif()

set(SOURCES
  src/main.c
  src/utils.c
)

set(HEADERS
  include/utils.h
)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
