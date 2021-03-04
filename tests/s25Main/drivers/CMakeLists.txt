# Testing driver system
add_testcase(NAME drivers
    LIBS s25Main testHelpers rttr::vld
)

add_dependencies(Test_drivers drivers)
