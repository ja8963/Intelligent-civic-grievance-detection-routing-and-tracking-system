cmake_minimum_required(VERSION 3.16)
project(CivicGrievanceGUI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

include_directories("C:/Program Files/MySQL/MySQL Server 8.0/include")

add_executable(CivicGrievanceGUI
    main.cpp
    mainwindow.cpp
    mainwindow.h
    logindialog.cpp
    logindialog.h
    complaint.cpp
    complaint.h
    databasemanager.cpp
    databasemanager.h
    user.h
)

target_link_libraries(CivicGrievanceGUI PRIVATE
    Qt6::Widgets
    Qt6::Sql
    "C:/Program Files/MySQL/MySQL Server 8.0/lib/libmysql.lib"
)

if(WIN32)
    set_target_properties(CivicGrievanceGUI PROPERTIES WIN32_EXECUTABLE FALSE)
endif()
