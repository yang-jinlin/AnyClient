cmake_minimum_required(VERSION 3.6)

project(AnyClient
	VERSION 1.0.0
	LANGUAGES C CXX
)

if (CYGWIN)
	message(FATAL_ERROR "Sorry, DO NOT support Cygwin")
endif ()

if (MINGW)
	message(FATAL_ERROR "Sorry, DO NOT support MinGW")
endif ()

include(GNUInstallDirs)

set(CMAKE_CONFIG_INSTALL_FILE ${PROJECT_BINARY_DIR}/config.toinstall.cmake)
set(CMAKE_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
set(INC_DIR ${PROJECT_SOURCE_DIR}/_include CACHE PATH "workflow inc")
set(LIB_DIR ${PROJECT_SOURCE_DIR}/_lib CACHE PATH "workflow lib")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LIB_DIR})
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${LIB_DIR})

if (WIN32)
	set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   /MP /wd4200")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /wd4200 /Zc:__cplusplus /std:c++14")
else ()
	set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -fPIC -pipe -std=gnu90")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -pipe -std=c++11 -fno-exceptions")
endif ()

add_subdirectory(workflow)
add_subdirectory(src)

add_library(
	${PROJECT_NAME} STATIC
	$<TARGET_OBJECTS:facility>
	$<TARGET_OBJECTS:kernel>
	$<TARGET_OBJECTS:util>
	$<TARGET_OBJECTS:manager>
	$<TARGET_OBJECTS:algorithm>
	$<TARGET_OBJECTS:protocol>
	$<TARGET_OBJECTS:factory>
	$<TARGET_OBJECTS:server>
	$<TARGET_OBJECTS:client>
)

add_dependencies(${PROJECT_NAME} workflow)

install(
	TARGETS ${PROJECT_NAME}
	ARCHIVE
	DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT devel
)

if (APPLE)
	set(LIBSO ${LIB_DIR}/libanyclient.a)
elseif (NOT WIN32)
	set(LIBSO ${LIB_DIR}/libanyclient.so)
	add_custom_target(
		SCRIPT_SHARED_LIB ALL
		COMMAND ${CMAKE_COMMAND} -E echo 'GROUP ( libanyclient.a AS_NEEDED ( libpthread.so libssl.so libcrypto.so ) ) ' > ${LIBSO}
	)
	add_dependencies(SCRIPT_SHARED_LIB ${PROJECT_NAME})
endif ()

install(
	FILES ${LIBSO}
	DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT devel
)

