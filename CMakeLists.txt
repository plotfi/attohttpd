project(attohttp)
add_executable(attohttpd attohttp.cpp)
set(CMAKE_CXX_FLAGS "-std=c++17 -pthread")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
