# Set name of the module
SET(MODULE realsense)

# Set all source files module uses
SET(SRC RealSense.cpp
        RealSense.h)


include_directories(${REALSENSE2_INCLUDE_DIR})
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME} ${REALSENSE2_LIBRARIES})

YURI_INSTALL_MODULE(${MODULE})
