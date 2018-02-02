cmake_minimum_required(VERSION 3.10)
project(http)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpthread")

find_package(Boost REQUIRED COMPONENTS system)

file(GLOB BOOST_ASIO_HTTP
        "server/*.cpp" "server/*.hpp"
        "client/*.cpp" "client/*.hpp")

add_executable(http ${BOOST_ASIO_HTTP} args_serializer.h main.cpp)

target_link_libraries(http ${Boost_SYSTEM_LIBRARY})