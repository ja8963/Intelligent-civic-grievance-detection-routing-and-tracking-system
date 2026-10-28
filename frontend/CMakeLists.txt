cmake_minimum_required(VERSION 3.16)
project(CivicGrievanceGUI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(CivicGrievanceGUI
    main.cpp
    mainwindow.cpp
    mainwindow.h
    logindialog.cpp
    logindialog.h
    complaint.cpp
    complaint.h
)

target_link_libraries(CivicGrievanceGUI PRIVATE Qt6::Widgets)

# On Windows do not open a console window alongside the GUI
if(WIN32)
    set_target_properties(CivicGrievanceGUI PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
