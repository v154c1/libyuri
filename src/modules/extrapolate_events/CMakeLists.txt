# Set name of the module
SET (MODULE extrapolate_events)

# Set all source files module uses
SET (SRC ExtrapolateEvents.cpp
		 ExtrapolateEvents.h)


 
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME})

YURI_INSTALL_MODULE(${MODULE})
