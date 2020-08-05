cmake_minimum_required(VERSION 3.6)

set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "build type")

project(anyclient_tutorial
		LANGUAGES C CXX
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

find_package(workflow REQUIRED CONFIG HINTS ../workflow)
find_package(anyclient REQUIRED CONFIG HINTS ..)
include_directories(${ANYCLIENT_INCLUDE_DIR} ${WORKFLOW_INCLUDE_DIR})
link_directories(${ANYCLIENT_LIB_DIR} ${WORKFLOW_LIB_DIR})

if (WIN32)
		set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   /MP /wd4200")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /wd4200 /std:c++14")
else ()
		set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -fPIC -pipe -std=gnu90")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -pipe -std=c++11 -fno-exceptions")
endif ()

set(TUTORIAL_LIST
	01_httpclient
	02_redisclient
	03_mysqlclient
	10_fileio
	20_timer
	30_counter
	40_go
)

if (APPLE)
	set(TT_LIB anyclient pthread OpenSSL::SSL OpenSSL::Crypto)
else ()
	set(TT_LIB anyclient ${LIBRT})
endif ()

foreach(src ${TUTORIAL_LIST})
	string(REPLACE "_" ";" arr ${src})
	list(GET arr -1 bin_name)
	add_executable(${bin_name} ${src}.cc)
	target_link_libraries(${bin_name} ${TT_LIB})
endforeach()

