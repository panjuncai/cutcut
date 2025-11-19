cmake_minimum_required(VERSION 3.21)

project(cutcut LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Gui)

set(PROJECT_SOURCES
    src/main.cpp
    src/appcontroller.h
    src/appcontroller.cpp
    src/screenshotoverlay.h
    src/screenshotoverlay.cpp
    src/pinnedshot.h
    src/pinnedshot.cpp
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Carbon")
endif()
