# Tests using the video system

add_subdirectory(uiHelper)

add_testcase(NAME UI
    LIBS s25Main testUIHelper
    COST 45
)
