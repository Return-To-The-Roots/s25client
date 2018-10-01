file(GLOB COMMON_SRC *.cpp)
file(GLOB COMMON_HEADERS include/*.h)
file(GLOB COMMON_HELPERS_SRC helpers/*.cpp)
file(GLOB COMMON_HELPERS_HEADERS include/helpers/*.h)
set(ALL_SRC ${COMMON_SRC} ${COMMON_HEADERS} ${COMMON_HELPERS_SRC} ${COMMON_HELPERS_HEADERS})

SOURCE_GROUP(src FILES ${COMMON_SRC} ${COMMON_HEADERS})
SOURCE_GROUP(helpers FILES ${COMMON_HELPERS_SRC} ${COMMON_HELPERS_HEADERS})

include_directories(SYSTEM ${Boost_INCLUDE_DIR})
add_library(s25Common STATIC ${ALL_SRC})
target_include_directories(s25Common PUBLIC include)
target_link_libraries(s25Common s25util)
SET_PROPERTY(TARGET s25Common PROPERTY POSITION_INDEPENDENT_CODE ON)

if(HAVE_STRUCT_TIMESPEC)
	target_compile_definitions(s25Common PUBLIC -DHAVE_STRUCT_TIMESPEC)
endif()

if(ClangFormat_FOUND)
    add_ClangFormat_files(${ALL_SRC})
endif()
