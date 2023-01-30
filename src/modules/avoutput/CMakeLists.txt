# Set name of the module
SET (MODULE avoutput)

# Set all source files module uses
SET (SRC AVOutput.cpp
		 AVOutput.h
		 register.cpp )


SET(CMAKE_REQUIRED_LIBRARIES ${libavutil_LIBRARIES})
SET(CMAKE_REQUIRED_FLAGS "${CMAKE_C_FLAGS}")
check_library_exists(avcodec av_frame_get_channels ${libavcodec_LIBDIR} HAVE_FRAME_GET_CHANNELS)
SET(CMAKE_REQUIRED_LIBRARIES )
SET(CMAKE_REQUIRED_FLAGS "")
if (NOT HAVE_FRAME_GET_CHANNELS)
    add_definitions(-DBROKEN_FFMPEG)
endif()

include_directories(${libavcodec_INCLUDE_DIRS})
link_directories(${libavcodec_LIBRARY_DIRS})
# You shouldn't need to edit anything below this line 
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME}  ${LIBNAME_HELPER_LIBAV} ${libavformat_LIBRARIES} ${libswscale_LIBRARIES})

YURI_INSTALL_MODULE(${MODULE})
