project(app3)
set(TARGET_NAME "${PROJECT_NAME}")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

set(sources main.cpp ../id.h)

add_executable(${TARGET_NAME} ${sources})
target_link_libraries(${TARGET_NAME} LINK_PRIVATE Qt${QT_VERSION_MAJOR}::Core sharedio)