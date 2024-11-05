project(FFZKit)
cmake_minimum_required(VERSION 3.1.3)


message(STATUS "CMake Version: ${CMAKE_VERSION}")

message(STATUS "PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")

#设置库文件路径
#Set the library file path
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
#设置可执行程序路径
#Set the executable program path
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
#设置子目录
#Set subdirectories
set(SUB_DIR_LIST "Network" "Poller" "Thread" "Util")

#使能c++11
#Enable c++11
set(CMAKE_CXX_STANDARD 11)

foreach(SUB_DIR ${SUB_DIR_LIST})
    aux_source_directory(src/${SUB_DIR} ${SUB_DIR})
endforeach()


#编译静态库
#Compile static library
add_library(${PROJECT_NAME}_static STATIC ${SRC_LIST})
#引用头文件路径
#Reference header file path
target_include_directories(${PROJECT_NAME}_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_target_properties(${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")

#测试程序
#Test program
if (NOT IOS)
    add_subdirectory(tests)
endif ()
