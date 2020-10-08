# Set name of the module
SET (MODULE pulse)

# Set all source files module uses
SET (SRC PulseOutput.cpp
		 PulseOutput.h
		 PulseInput.cpp
		 PulseInput.h
		 register.cpp)

 
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME} ${PULSEAUDIO_LIBRARY} ${PULSEAUDIO_SIMPLE_LIBRARY})

YURI_INSTALL_MODULE(${MODULE})
