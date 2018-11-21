add_subdirectory(mockupDrivers)

find_package(Boost REQUIRED COMPONENTS unit_test_framework)
if(NOT Boost_USE_STATIC_LIBS)
    ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
endif()
if(MSVC)
    # Use full paths for defines (makes Boost.Test with the VS addin work better)
    add_definitions(/FC)
endif()

add_subdirectory(s25Main)
