set(DRIVER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/driver)
INCLUDE_DIRECTORIES(${DRIVER_INCLUDE_DIR})

ADD_SUBDIRECTORY(src)

include_directories(src)

# Video drivers
rttr_set_output_dir(RUNTIME ${RTTR_DRIVERDIR}/video)
rttr_set_output_dir(LIBRARY ${RTTR_DRIVERDIR}/video)
ADD_SUBDIRECTORY(video) 

# Audio drivers
rttr_set_output_dir(RUNTIME ${RTTR_DRIVERDIR}/audio)
rttr_set_output_dir(LIBRARY ${RTTR_DRIVERDIR}/audio)
ADD_SUBDIRECTORY(audio) 
