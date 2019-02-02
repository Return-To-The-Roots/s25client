rttr_set_output_dir(RUNTIME ${RTTR_DRIVERDIR}/video)
rttr_set_output_dir(LIBRARY ${RTTR_DRIVERDIR}/video)

add_subdirectory(WinAPI)
add_subdirectory(SDL)
add_subdirectory(SDL2)
