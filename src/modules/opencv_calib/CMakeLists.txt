# Set name of the module
SET (MODULE opencv_calib)

# Set all source files module uses
SET (SRC OpenCVCalib.cpp
		 OpenCVCalib.h)



# You shouldn't need to edit anything below this line 
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME} ${OpenCV_LIBRARIES})

YURI_INSTALL_MODULE(${MODULE})
