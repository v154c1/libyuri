# Set name of the module
SET (MODULE draw)

# Set all source files module uses
SET (SRC Box.cpp
		 Box.h
		 register.cpp)


 
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME})

YURI_INSTALL_MODULE(${MODULE})
