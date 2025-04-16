cmake_minimum_required(VERSION 3.30)
project(AskiLang)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(AskiLang
    arena.hpp
    generation.hpp
    main.cpp
    parser.hpp
    tokenization.hpp)
