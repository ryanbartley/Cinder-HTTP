if( NOT TARGET Cinder-HTTP )
	
	get_filename_component( CINDER_HTTP_INCLUDE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../src" ABSOLUTE )

	# Add OSC block as a dependency
	get_filename_component( CINDER_SSL_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../Cinder-SSL/proj/cmake" ABSOLUTE )
	find_package( Cinder-SSL REQUIRED PATHS "${CINDER_SSL_MODULE_PATH}" )
	set( Cinder-HTTP_INCLUDES ${CINDER_HTTP_INCLUDE_PATH} ) 

endif()
