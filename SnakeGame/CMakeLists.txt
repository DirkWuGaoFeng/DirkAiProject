cmake_minimum_required(VERSION 3.16)
project(SnakeGame VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(SnakeGame
    main.cpp
    snake.cpp
    snake.h
)

target_link_libraries(SnakeGame PRIVATE Qt6::Widgets)