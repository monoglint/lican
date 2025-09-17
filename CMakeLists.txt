cmake_minimum_required(VERSION 3.10)
project(licanc VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Your targets here...

add_executable(licanc
    src/main.cpp
    src/lex.cpp
    src/parse.cpp
    src/licanapi.cpp
    src/core.cpp
    src/semantic.cpp
    src/generate.cpp
    resources/resources.rc
)

# Add include directory
target_include_directories(licanc PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Set C++ standard
set_property(TARGET licanc PROPERTY CXX_STANDARD 17)