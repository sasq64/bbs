cmake_minimum_required(VERSION 2.8.4)
project(bbs)

set(AP1_ROOT ../apone)
include(${AP1_ROOT}/cmake/Utils.cmake)

add_definitions(-DAPP_NAME=bbs)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -DCM_DEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -DCM_DEBUG")


include_directories(${AP1_ROOT}/mods)

set(SOURCE_FILES bbs.cpp loginmanager.cpp messageboard.cpp comboard.cpp bbstools.cpp main.cpp)
#set(SOURCE_FILES bbstest.cpp)

set(APONE_MODULES
	coreutils
	bbsutils
	archive
	luainterpreter
	sqlite3
	crypto
)

add_subdirectory(${AP1_ROOT}/mods ap1mods)

add_executable(petcat petcat.cpp)
target_link_libraries(petcat coreutils)

add_executable(bbs ${SOURCE_FILES})
target_link_libraries(bbs ${APONE_MODULES})

