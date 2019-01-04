# Library containing old files that are still useful for tests
add_library(legacyTestfiles STATIC src/legacy/TerrainData.cpp)
target_include_directories(legacyTestfiles PUBLIC src)
target_link_libraries(legacyTestfiles PUBLIC s25Main)
