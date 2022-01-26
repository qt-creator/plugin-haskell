cmake_minimum_required(VERSION 3.10)

project(Haskell)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)

find_package(QtCreator COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_subdirectory(plugins/haskell)
add_subdirectory(tests/auto/tokenizer)
