cmake_minimum_required(VERSION 3.15)

project(Castr
  VERSION 1.0.0
  DESCRIPTION "Castr"
  LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(vendor/glfw)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_definitions(CASTR_RELEASE)
endif()

if (MSVC)
  add_compile_options(/W4)
  message(STATUS "Configuring for msvc compiler")
endif()

set(SOURCES
  src/main.c
  src/utils.c
  src/logger.c
  vendor/glad/src/glad.c
)

set(HEADERS
  include/utils.h
  include/logger.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/vendor/glfw/include
  ${PROJECT_SOURCE_DIR}/vendor/glad/include
)

if (MSVC)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11
    dxgi
    dxguid
  )
endif()

find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw OpenGL::GL)

set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)