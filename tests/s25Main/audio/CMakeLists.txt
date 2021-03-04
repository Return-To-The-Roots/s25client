# Tests using the audio system

add_testcase(NAME sounds
    LIBS s25Main audioMockup testHelpers testConfig rttr::vld
)
