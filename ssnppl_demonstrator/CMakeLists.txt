cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(ssnppl_demonstrator LANGUAGES C CXX)

find_package(Boost COMPONENTS program_options thread  REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

set(CMAKE_BUILD_TYPE Release)

#Check PPL Lib
find_library(PPL_LIB_PATH "libpointperfect.a" PATHS ${CMAKE_SOURCE_DIR}/PPL/lib REQUIRED)

add_executable(ssnppl_demonstrator src/main.cpp src/ssnppl.cpp src/SerialComm.cpp src/program_option.cpp src/mqtt.cpp src/utils.cpp)

target_include_directories(ssnppl_demonstrator PRIVATE ${CMAKE_SOURCE_DIR}/include ${Boost_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/PPL/inc)
target_link_libraries(ssnppl_demonstrator PRIVATE Boost::program_options Threads::Threads Boost::thread mosquitto ${PPL_LIB_PATH})

add_compile_options("-Wall")
