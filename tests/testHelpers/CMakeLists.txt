add_library(testHelpers STATIC
    rttr/test/BaseFixture.cpp
    rttr/test/BaseFixture.hpp
    rttr/test/BufferedWriter.hpp
    rttr/test/LocaleResetter.hpp
    rttr/test/LogAccessor.hpp
    rttr/test/Fixture.cpp
    rttr/test/Fixture.hpp
    rttr/test/testHelpers.hpp
)
target_link_libraries(testHelpers PRIVATE s25Common rttrConfig)
target_include_directories(testHelpers PUBLIC .)
    