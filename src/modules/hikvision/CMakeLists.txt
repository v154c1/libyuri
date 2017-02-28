# Set name of the module
SET (MODULE hikvision)

# Set all source files module uses
SET (SRC HikVision.cpp
		 HikVision.h)


 
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME} hcnetsdk PlayCtrl)

YURI_INSTALL_MODULE(${MODULE})
