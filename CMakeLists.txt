cmake_minimum_required(VERSION 3.21)

project(cutcut LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Gui Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Gui Svg)

set(PROJECT_SOURCES
    assets/icons.qrc
    src/main.cpp
    src/appcontroller.h
    src/appcontroller.cpp
    src/screenimage.h
    src/screenshotoverlay.h
    src/screenshotoverlay.cpp
    src/pinnedshot.h
    src/pinnedshot.cpp
    src/selectiontoolbar.h
    src/selectiontoolbar.cpp
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    target_sources(${PROJECT_NAME} PRIVATE installer/cutcut.rc)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Svg)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Carbon" "-framework CoreGraphics")
endif()

option(CUTCUT_BUILD_TESTS "Build screenshot pixel-preservation tests" OFF)
if(CUTCUT_BUILD_TESTS)
    enable_testing()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
    add_executable(screenimage_tests
        tests/screenimage_tests.cpp
        src/screenshotoverlay.cpp src/screenshotoverlay.h
        src/selectiontoolbar.cpp src/selectiontoolbar.h
        src/pinnedshot.cpp src/pinnedshot.h
    )
    target_include_directories(screenimage_tests PRIVATE src)
    target_link_libraries(screenimage_tests PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg Qt${QT_VERSION_MAJOR}::Test)
    add_test(NAME screenimage_tests COMMAND screenimage_tests)
    set_tests_properties(screenimage_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
