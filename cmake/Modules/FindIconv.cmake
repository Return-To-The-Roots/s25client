# - Try to find Iconv 
# Once done this will define 
# 
#  ICONV_FOUND - system has Iconv 
#  ICONV_INCLUDE_DIR - the Iconv include directory 
#  ICONV_LIBRARY - Link these to use Iconv 
# 

SET(ICONV_DIR_SEARCH $ENV{ICONV_ROOT})

FIND_PATH(ICONV_INCLUDE_DIR iconv.h 
  ${ICONV_DIR_SEARCH}/include	
  /usr/include 
  /usr/local/include 
) 
 
FIND_LIBRARY(ICONV_LIBRARY NAMES iconv c
  PATHS 
  ${ICONV_DIR_SEARCH}/lib
  /usr/${LIB_DESTINATION}
  /usr/local/${LIB_DESTINATION}) 
 
IF(ICONV_INCLUDE_DIR AND ICONV_LIBRARY) 
   SET(Iconv_FOUND TRUE) 
ENDIF(ICONV_INCLUDE_DIR AND ICONV_LIBRARY) 
 
IF(Iconv_FOUND) 
  IF(NOT Iconv_FIND_QUIETLY) 
    MESSAGE(STATUS "Found Iconv: ${ICONV_LIBRARY}") 
  ENDIF(NOT Iconv_FIND_QUIETLY) 
ELSE(Iconv_FOUND) 
  IF(Iconv_FIND_REQUIRED) 
    MESSAGE(FATAL_ERROR "Could not find Iconv") 
  ENDIF(Iconv_FIND_REQUIRED) 
ENDIF(Iconv_FOUND) 

MARK_AS_ADVANCED(
  ICONV_INCLUDE_DIR
  ICONV_LIBRARY
)
