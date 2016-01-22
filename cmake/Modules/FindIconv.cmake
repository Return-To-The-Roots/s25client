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

FIND_LIBRARY(ICONV_LIBRARY NAMES iconv libiconv2 c
  PATHS 
  ${ICONV_DIR_SEARCH}/lib
  /usr/${LIB_DESTINATION}
  /usr/local/${LIB_DESTINATION}) 
 
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(IConv DEFAULT_MSG ICONV_INCLUDE_DIR ICONV_LIBRARY)

MARK_AS_ADVANCED(
  ICONV_INCLUDE_DIR
  ICONV_LIBRARY
)
