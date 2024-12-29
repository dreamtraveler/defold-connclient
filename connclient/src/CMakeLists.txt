cmake_minimum_required(VERSION 3.21)

project(connclient VERSION 1.0.0)

message(STATUS "CMAKE_C_COMPILER: " ${CMAKE_C_COMPILER})
message(STATUS "CMAKE_CXX_COMPILER: " ${CMAKE_CXX_COMPILER})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# cmake内置变量使用大写，其他匀用小写

# 编译参数
set(CMAKE_C_FLAGS "-Wall -Werror -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra -pipe -Wno-unused-parameter -Wno-unused-value -Wno-unused-local-typedefs -Wno-deprecated-declarations -Wno-sign-compare")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0")
endif(APPLE)

set(CMAKE_CXX_STANDARD 20)
# 编译器不支持C++20则报错
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# OFF关闭GCC的扩展功能，只使用标准的C++
set(CMAKE_CXX_EXTENSIONS OFF) 
message(STATUS "CMAKE_C_FLAGS " ${CMAKE_C_FLAGS})
message(STATUS "CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS})

# 预定义
add_definitions(-D_GNU_SOURCE -D_REENTRANT)

#设置变量
set(lib_dir ${PROJECT_SOURCE_DIR}/lib)

# 头文件包含路径
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)
aux_source_directory(. src)
list(REMOVE_ITEM src "./main.cpp")
list(REMOVE_ITEM src "./myextension.cpp")

message(STATUS "CMAKE_CURRENT_SOURCE_DIR " ${CMAKE_CURRENT_SOURCE_DIR})
#编译库存放路径
set(LIBRARY_OUTPUT_PATH ${lib_dir})
add_library(connclient STATIC ${src})
