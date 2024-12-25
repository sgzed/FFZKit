project(FFZKit)
cmake_minimum_required(VERSION 3.10.0)

message(STATUS "CMake Version: ${CMAKE_VERSION}")

message(STATUS "PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE "Debug")
endif()

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

#设置库文件路径
#Set the library file path
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
#设置可执行程序路径
#Set the executable program path
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
#设置子目录
#Set subdirectories
set(SUB_DIR_LIST "Network" "Poller" "Thread" "Util")

if(WIN32)
    list(APPEND SUB_DIR_LIST "win32")
    #防止Windows.h包含Winsock.h
    #Prevent Windows.h from including Winsock.h
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    if (MSVC)
        add_compile_options("/utf-8")
    endif()
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

#使能c++11
#Enable c++11
set(CMAKE_CXX_STANDARD 11)

foreach(SUB_DIR ${SUB_DIR_LIST})
    aux_source_directory(src/${SUB_DIR} SRC_LIST)
endforeach()

if (WIN32)
    set(LINK_LIB_LIST WS2_32 Iphlpapi shlwapi)
else ()
    set(LINK_LIB_LIST)
endif ()

if (NOT WIN32)
    add_compile_options(-Wno-deprecated-declarations)
    add_compile_options(-Wno-predefined-identifier-outside-function)
endif ()

#编译动态库
#Compile dynamic library
if (NOT IOS AND NOT ANDROID AND NOT WIN32)
    add_library(${PROJECT_NAME}_shared SHARED ${SRC_LIST})
    target_include_directories(${PROJECT_NAME}_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(${PROJECT_NAME}_shared ${LINK_LIB_LIST})
    set_target_properties(${PROJECT_NAME}_shared PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
    install(TARGETS ${PROJECT_NAME}_shared ARCHIVE DESTINATION ${INSTALL_PATH_LIB} LIBRARY DESTINATION ${INSTALL_PATH_LIB})
endif ()

#编译静态库
#Compile static library
add_library(${PROJECT_NAME}_static STATIC ${SRC_LIST})
#引用头文件路径
#Reference header file path
target_include_directories(${PROJECT_NAME}_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_target_properties(${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
#安装静态库至系统目录
#Install static library to system directory
install(TARGETS ${PROJECT_NAME}_static ARCHIVE DESTINATION ${INSTALL_PATH_LIB})


#测试程序
#Test program
if (NOT IOS)
    add_subdirectory(tests)
endif ()
