# Note: Sources and headers are generated using http://glad.dav1d.de/ on 07.07.2018
# See glad.h for details

add_library(glad STATIC src/glad.c)
target_include_directories(glad PUBLIC include)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	TARGET_LINK_LIBRARIES(glad PUBLIC ${CMAKE_DL_LIBS})
endif()
