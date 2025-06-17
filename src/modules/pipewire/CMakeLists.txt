# Set name of the module
SET (MODULE pipewire)

# Set all source files module uses
SET (SRC PipewireInput.cpp
         PipewireInput.h
         PipewireOutput.cpp
		 PipewireOutput.h
		 register.cpp)

include_directories(${SPA_INCLUDE_DIRS})
include_directories(${PIPEWIRE_INCLUDE_DIRS})
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME} PkgConfig::PIPEWIRE PkgConfig::SPA)

YURI_INSTALL_MODULE(${MODULE})
