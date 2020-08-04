cmake_minimum_required(VERSION 3.6)

project(anyclient
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
set(INC_DIR ${PROJECT_SOURCE_DIR}/_include CACHE PATH "anyclient inc")
set(LIB_DIR ${PROJECT_SOURCE_DIR}/_lib CACHE PATH "anyclient lib")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LIB_DIR})
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${LIB_DIR})

add_custom_target(
	LINK_HEADERS ALL
	COMMENT "link headers..."
)

set(INCLUDE_HEADERS
	src/WFHttpClient.h
	src/WFMySQLClient.h
	src/WFRedisClient.h
	src/WFWebServer.h
	src/WFWebServer.inl
)

macro(makeLink src dest target)
	add_custom_command(
		TARGET ${target} PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${src} ${dest}
		DEPENDS ${dest}
	)
endmacro()

add_custom_command(
	TARGET LINK_HEADERS PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory ${INC_DIR}/${PROJECT_NAME}
)

foreach(header_file ${INCLUDE_HEADERS} ${INCLUDE_KERNEL_HEADERS})
	#file(COPY ${PROJECT_SOURCE_DIR}/${header_file} DESTINATION ${INC_DIR}/${PROJECT_NAME})
	string(REPLACE "/" ";" arr ${header_file})
	list(GET arr -1 file_name)
	makeLink(${PROJECT_SOURCE_DIR}/${header_file} ${INC_DIR}/${PROJECT_NAME}/${file_name} LINK_HEADERS)
endforeach()

if (WIN32)
	set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   /MP /wd4200")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /wd4200 /Zc:__cplusplus /std:c++14")
else ()
	set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall -fPIC -pipe -std=gnu90")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -pipe -std=c++11 -fno-exceptions")
endif ()

find_package(workflow REQUIRED CONFIG HINTS workflow)
include_directories(${INC_DIR}/anyclient ${WORKFLOW_INCLUDE_DIR})
link_directories(${WORKFLOW_LIB_DIR})

add_subdirectory(src)

add_library(
	${PROJECT_NAME} STATIC
	$<TARGET_OBJECTS:facility>
)

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
		COMMAND ${CMAKE_COMMAND} -E echo 'GROUP ( libanyclient.a AS_NEEDED ( libworkflow.so ) ) ' > ${LIBSO}
	)
	add_dependencies(SCRIPT_SHARED_LIB ${PROJECT_NAME})
endif ()

install(
	FILES ${LIBSO}
	DESTINATION ${CMAKE_INSTALL_LIBDIR}
	COMPONENT devel
)

####CONFIG

include(CMakePackageConfigHelpers)
set(CONFIG_INC_DIR ${INC_DIR})
set(CONFIG_LIB_DIR ${LIB_DIR})
configure_package_config_file(
	${PROJECT_NAME}-config.cmake.in
	${PROJECT_SOURCE_DIR}/${PROJECT_NAME}-config.cmake
	INSTALL_DESTINATION ${CMAKE_CONFIG_INSTALL_DIR}
	PATH_VARS CONFIG_INC_DIR CONFIG_LIB_DIR
)

set(CONFIG_INC_DIR ${CMAKE_INSTALL_INCLUDEDIR})
set(CONFIG_LIB_DIR ${CMAKE_INSTALL_LIBDIR})
configure_package_config_file(
	${PROJECT_NAME}-config.cmake.in
	${CMAKE_CONFIG_INSTALL_FILE}
	INSTALL_DESTINATION ${CMAKE_CONFIG_INSTALL_DIR}
	PATH_VARS CONFIG_INC_DIR CONFIG_LIB_DIR
)

install(
	FILES ${CMAKE_CONFIG_INSTALL_FILE}
	DESTINATION ${CMAKE_CONFIG_INSTALL_DIR}
	COMPONENT devel
	RENAME ${PROJECT_NAME}-config.cmake
)

#### PACK

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C++ AnyClient")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})

# set(CPACK_RPM_PRE_INSTALL_SCRIPT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/pre_script.sh)
# set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/post_script.sh)

execute_process(
	COMMAND git log -n1 --format=%ad --date=short
	RESULT_VARIABLE GIT_LOG_RET
	OUTPUT_VARIABLE GIT_YMD
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (NOT GIT_LOG_RET EQUAL 0)
	set(GIT_YMD 0)
endif()

string(REPLACE "-" "" RELEASE_LABEL ${GIT_YMD})
set(CPACK_RPM_PACKAGE_RELEASE ${RELEASE_LABEL})

set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_ALL devel)
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_RELEASE_DIST ON)

set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /usr/lib64/cmake)

set(CPACK_RPM_DEVEL_PACKAGE_REQUIRES "workflow-devel >= 1.12.5")

install(
	FILES ${INCLUDE_HEADERS}
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
	COMPONENT devel
)

install(
	FILES README.md
	DESTINATION "${CMAKE_INSTALL_DOCDIR}-${PROJECT_VERSION}"
	COMPONENT devel
)

set(CPACK_RPM_SPEC_MORE_DEFINE "%define __spec_install_post /bin/true") # disable strip
set(CPACK_GENERATOR "RPM")

include(CPack)

