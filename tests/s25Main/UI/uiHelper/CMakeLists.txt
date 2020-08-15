add_library(testUIHelper STATIC
    uiHelper/uiHelpers.hpp
    uiHelper/uiHelpers.cpp
)
target_include_directories(testUIHelper PUBLIC .)
target_link_libraries(testUIHelper PUBLIC s25Main videoMockup testHelpers turtle)
enable_warnings(testUIHelper)
