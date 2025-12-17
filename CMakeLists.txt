project(FFZKit)
cmake_minimum_required(VERSION 3.1.3...3.26)

message(STATUS "CMake Version: ${CMAKE_VERSION}")

message(STATUS "PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE "Debug")
endif()

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

string(TOLOWER ${CMAKE_SYSTEM_NAME} SYSTEM_NAME_LOWER)

#使能c++11
#Enable c++11
set(CMAKE_CXX_STANDARD 11)

# -fPIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# 打印详情
set(CMAKE_VERBOSE_MAKEFILE ON)

#加载自定义模块
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(ENABLE_WEPOLL "Enable wepoll" ON)
# shared library by default
option(BUILD_SHARED_LIBS "Build all libraries shared" ON)

# 方便修改全局变量
function(update_cached name value)
    set("${name}" "${value}" CACHE INTERNAL "*** Internal ***" FORCE)
endfunction()

function(update_cached_list name)
    set(_tmp_list "${${name}}")
    list(APPEND _tmp_list "${ARGN}")
    list(REMOVE_DUPLICATES _tmp_list)
    update_cached(${name} "${_tmp_list}")
endfunction()

update_cached(TK_INC_PATHS "")
update_cached(TK_LINK_LIBRARIES "")
update_cached(TK_COMPILE_DEFINITIONS "")
update_cached(TK_COMPILE_OPTIONS "")

# 收集源码
file(GLOB SRC_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*/*/*.cpp)

if (WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    if (MSVC)
        update_cached_list(TK_COMPILE_OPTIONS "/utf-8")
    endif ()
    update_cached_list(TK_LINK_LIBRARIES WS2_32 Iphlpapi shlwapi)
    #防止Windows.h包含Winsock.h
    update_cached_list(TK_COMPILE_DEFINITIONS WIN32_LEAN_AND_MEAN MP4V2_NO_STDINT_DEFS _CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS)
else ()
    # 非Windows平台自带getopt api
    list(FILTER SRC_LIST EXCLUDE REGEX "getopt.c$")
    update_cached_list(TK_COMPILE_OPTIONS "-Wno-comment" "-Wno-deprecated-declarations" "-Wno-predefined-identifier-outside-function")
endif ()

if (NOT WIN32 OR NOT ENABLE_WEPOLL)
    # 移除wepoll
    list(FILTER SRC_LIST EXCLUDE REGEX "wepoll.c$")
else ()
    update_cached_list(TK_COMPILE_DEFINITIONS HAS_EPOLL)
    update_cached_list(TK_INC_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/src/win32/)
endif ()

#非苹果平台移除.mm类型的文件
if (NOT APPLE)
    list(FILTER SRC_LIST EXCLUDE REGEX "Socket_ios.mm$")
endif ()

# 库依赖
add_library(${PROJECT_NAME}_deps INTERFACE)
target_link_libraries(${PROJECT_NAME}_deps INTERFACE ${TK_LINK_LIBRARIES})

#编译库
add_library(${PROJECT_NAME} ${SRC_LIST})
#引用头文件路径
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${TK_INC_PATHS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${PROJECT_NAME}_deps)
target_compile_definitions(${PROJECT_NAME} PUBLIC ${TK_COMPILE_DEFINITIONS})
target_compile_options(${PROJECT_NAME} PUBLIC ${TK_COMPILE_OPTIONS})
set_target_properties(${PROJECT_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib
                                                 ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    #本工程为root工程，则添加测试程序
    add_subdirectory(tests)
endif ()