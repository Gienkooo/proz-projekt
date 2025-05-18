cmake_minimum_required(VERSION 3.10)
project(proz_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(MPI REQUIRED)
find_package(Threads REQUIRED)

add_executable(proz_sim
    main.cpp
    MessageHandler.cpp
    ResourceManager.cpp
    ProcessLogic.cpp
)

target_include_directories(proz_sim PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MPI_CXX_INCLUDE_DIRS}
)

target_link_libraries(proz_sim PUBLIC
    MPI::MPI_CXX
    Threads::Threads
)

if (MSVC)
    target_compile_options(proz_sim PRIVATE "/EHsc" "/std:c++17")
else()
    target_compile_options(proz_sim PRIVATE "-std=c++17" "-Wall" "-Wextra" "-pthread")
endif()

target_compile_features(proz_sim PUBLIC cxx_std_17)