cmake_minimum_required(VERSION 3.5)

project(Graph LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(Graph WIN32
wres.rc
main.cpp
mainwindow.cpp
mainwindow.h
mainwindow.ui
scaletranslatedialog.cpp
scaletranslatedialog.h
scaletranslatedialog.ui
functiongenerator.cpp
functiongenerator.h
functiongenerator.ui
graph.cpp
graph.h
res.qrc
)

target_link_libraries(Graph PRIVATE Qt5::Widgets)
