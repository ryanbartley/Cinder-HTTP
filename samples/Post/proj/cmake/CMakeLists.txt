cmake_minimum_required( VERSION 3.0 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( Post )

get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../.." ABSOLUTE )
get_filename_component( APP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../" ABSOLUTE )
get_filename_component( BLOCKS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE )

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

set( SRC_FILES ${APP_PATH}/src/PostApp.cpp )
set( HEADER_FILES ${BLOCKS_PATH}/src ${BLOCKS_PATH}/lib/include	)
set( SSL_LIBRARIES ${BLOCKS_PATH}/lib/linux/libssl.a ${BLOCKS_PATH}/lib/linux/libcrypto.a )

ci_make_app(
	SOURCES     ${SRC_FILES}
	CINDER_PATH ${CINDER_PATH}
	INCLUDES    ${HEADER_FILES}
	LIBRARIES   ${SSL_LIBRARIES}
)

# FIXME: why aren't these different when building out of source?
message( "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}" )
message( "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}" )
