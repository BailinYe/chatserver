cmake_minimum_required(VERSION 3.0)
project(ChatServer)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)                           
set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_FLAGS} -g)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/include/server/db)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)

#加载子目录
add_subdirectory(src)

