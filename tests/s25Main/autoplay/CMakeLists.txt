# Tests running a whole simulation
# Example: Replay testing to make sure nothing introduced unexpected changes
add_testcase(NAME autoplay
    LIBS s25Main testConfig testHelpers
    CONFIGURATIONS Release RelWithDebInfo # This is really slow so only run when code is optimized
)
