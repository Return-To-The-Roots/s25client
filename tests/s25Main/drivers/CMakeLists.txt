# Testing driver system
add_testcase(NAME drivers
    LIBS s25Main testHelpers
)

add_dependencies(Test_drivers drivers)
