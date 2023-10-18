project(sharedio)

set(TARGET_NAME "${PROJECT_NAME}")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

file(GLOB_RECURSE headers "*.hpp")

set(sources
   src/sharedio.cpp
   src/sharedioprivate.cpp
   )

add_library(${TARGET_NAME} STATIC
  ${sources}
  ${headers}
)

target_include_directories(${TARGET_NAME}
  PRIVATE
    src
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(${TARGET_NAME} LINK_PRIVATE Qt${QT_VERSION_MAJOR}::Core )