# Tests testing more than single components
# e.g. creating a whole world
# Lua related tests are extra
add_testcase(NAME integration
    LIBS s25Main testHelpers testWorldFixtures testUIHelper
    COST 50
)
