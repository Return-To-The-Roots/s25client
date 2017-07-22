
ADD_SUBDIRECTORY( src )

#################################################################################
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/src)
ADD_SUBDIRECTORY( video ) 
ADD_SUBDIRECTORY( audio ) 

#################################################################################
