cmake_minimum_required(VERSION 3.16)
project(StockApp VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS
    Core
    Gui
    Widgets
    Network
    Charts
    REQUIRED
)

add_executable(StockApp
    main.cpp
    mainwindow.cpp
    mainwindow.h
    chartmanager.cpp
    chartmanager.h
    stockdatamanager.cpp
    stockdatamanager.h
)

target_link_libraries(StockApp PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Charts
)