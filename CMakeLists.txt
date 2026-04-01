cmake_minimum_required(VERSION 3.16)
project(XPlayer LANGUAGES CXX)

include(qt.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1)
add_compile_definitions(_SILENCE_CXX17_C_HEADER_DEPRECATION_WARNING)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR}
    COMPONENTS
        Core
        Gui
        Widgets
)
qt_standard_project_setup()

file(GLOB XPLAYER_SRC_FILES
    "src/*.cpp"
    "src/demuxer/*.cpp"
    "src/decoder/*.cpp"
    "src/renderer/*.cpp"
    "src/rescaler/*.cpp"
    "src/filter/*.cpp"
    "src/record/*.cpp"
    "src/utils/*.cpp"
)

file(GLOB XPLAYER_UI_FILES
    "ui/*.cpp"
    "ui/*.ui"
)

if(QT_VERSION VERSION_LESS 6.0)
    qt5_add_resources(QRC_FILES "resources/XPlayer.qrc")
else()
    qt6_add_resources(QRC_FILES "resources/XPlayer.qrc")
endif()

qt_add_executable(${PROJECT_NAME} 
                  ${XPLAYER_UI_FILES} 
                  ${XPLAYER_SRC_FILES}
                  ${QRC_FILES}
                  "resources/XPlayer.rc")

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        WIN32_EXECUTABLE TRUE
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    dep/include
    src/demuxer
    src/renderer
    src/rescaler
    src/decoder
    src/filter
    src/record
    src/utils
    src/)

target_link_directories(${PROJECT_NAME}
    PRIVATE
    dep/lib
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        avutil
        avformat
        avcodec
        swresample
        swscale
        SDL2
        SDL2_ttf
        jansson
        SoundTouch
        $<$<CONFIG:Debug>:FramelessHelperCore64d>
        $<$<CONFIG:Release>:FramelessHelperCore64>
        $<$<CONFIG:Debug>:FramelessHelperWidgets64d>
        $<$<CONFIG:Release>:FramelessHelperWidgets64>
)
