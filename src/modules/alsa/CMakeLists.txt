# Set name of the module
SET (MODULE alsa_module)

# Set all source files module uses
SET (SRC AlsaOutput.cpp
		 AlsaOutput.h
		 AlsaInput.cpp
		 AlsaInput.h
		 register.cpp)

 
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME} ${ALSA_LIBRARIES})

YURI_INSTALL_MODULE(${MODULE})
