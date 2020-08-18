# Set name of the module
SET (MODULE extrapolate_events)

# Set all source files module uses
SET (SRC ExtrapolateEvents.cpp
		 ExtrapolateEvents.h)


add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME})
target_compile_options(${MODULE} PUBLIC "-fno-finite-math-only")
YURI_INSTALL_MODULE(${MODULE})
