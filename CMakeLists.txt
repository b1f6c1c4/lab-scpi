cmake_minimum_required(VERSION 3.23)
project(lab-scpi VERSION 0.0)

set(CMAKE_CXX_STANDARD 23)

add_executable(lab
	main.cpp
	scpi.cpp
	profile.cpp
	scpi.cpp
	yaml.cpp
	fancy_cin.cpp
	executor.cpp
	formattor.cpp)
target_compile_options(lab PRIVATE -Wno-trigraphs)
target_include_directories(lab PRIVATE vendor)
target_include_directories(lab PRIVATE include)
target_link_libraries(lab PRIVATE m)
