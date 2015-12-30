CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

CMAKE_POLICY(SET CMP0021 OLD)

set(CXX_NORM_CXX98 1) # C++98
set(CXX_NORM_CXX03 2) # C++03
set(CXX_NORM_CXX11 3) # C++11

macro(set_cxx_norm NORM)
    exec_program(
        ${CMAKE_CXX_COMPILER}
        ARGS --version
        OUTPUT_VARIABLE _compiler_output
    )
    # Keep only the first line
    string(REGEX REPLACE
        "(\n.*$)"
        ""
        cxx_compiler_version "${_compiler_output}"
    )
    # Extract the version number
    string(REGEX REPLACE
        "([^0-9.])|([0-9.][^0-9.])"
        ""
        cxx_compiler_version "${cxx_compiler_version}"
    )
    if(CMAKE_COMPILER_IS_GNUCXX)
        if(${NORM} EQUAL ${CXX_NORM_CXX98})
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98")
        elseif(${NORM} EQUAL ${CXX_NORM_CXX03})
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++03")
        elseif(${NORM} EQUAL ${CXX_NORM_CXX11})
            if(${cxx_compiler_version} VERSION_LESS "4.7.0")
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
            else()
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
            endif()
        endif()
    elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
        if(${NORM} EQUAL ${CXX_NORM_CXX11})
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
        endif()
    endif()
endmacro()

PROJECT(SockChatServer)

FILE(GLOB SockLibSrc socklib/*.cpp socklib/*.c)
ADD_LIBRARY(socklib SHARED ${SockLibSrc})
SET_TARGET_PROPERTIES(socklib PROPERTIES COMPILE_FLAGS "-DSOCKLIB_EXPORT -DNULL=0")
IF(CMAKE_COMPILER_IS_GNUCXX)
    IF(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        TARGET_LINK_LIBRARIES(socklib dl pthread nsl resolv)
    ELSE()
        TARGET_LINK_LIBRARIES(socklib dl pthread nsl socket resolv)
    ENDIF()
ENDIF()

#FILE(GLOB CoreLibSrc core/*.cpp)
ADD_LIBRARY(core SHARED core/main.cpp)
SET_TARGET_PROPERTIES(core PROPERTIES COMPILE_FLAGS "-DLIBCORE_EXPORT -DNULL=0")
SET_TARGET_PROPERTIES(core PROPERTIES INCLUDE_DIRECTORIES "socklib/")
TARGET_LINK_LIBRARIES(core socklib)

FILE(GLOB ServerSrc sockchat/*.cpp)
ADD_EXECUTABLE(server ${ServerSrc})
SET_TARGET_PROPERTIES(server PROPERTIES COMPILE_FLAGS "-DNULL=0")
SET_TARGET_PROPERTIES(server PROPERTIES INCLUDE_DIRECTORIES "socklib/")
TARGET_LINK_LIBRARIES(server socklib)

ADD_DEPENDENCIES(core socklib)
ADD_DEPENDENCIES(server core socklib)

set_cxx_norm(CXX_NORM_CXX11)