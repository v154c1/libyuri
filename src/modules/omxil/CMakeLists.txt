# Set name of the module
SET (MODULE omxil)

# Set all source files module uses
SET (SRC OmxilOutput.cpp
		 OmxilOutput.h 
		 register.cpp)

# You shouldn't need to edit anything below this line
include_directories(${OMXIL_INCLUDE_DIRS}) 
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME} ${OMXIL_LIBRARIES})

YURI_INSTALL_MODULE(${MODULE})
