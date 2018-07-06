# Set name of the module
SET (MODULE imagemagick_source)

# Set all source files module uses
SET (SRC ImageMagickSource.cpp
		 ImageMagickSource.h)


include_directories(${ImageMagick_Magick++_INCLUDE_DIR})

IF(ImageMagick_Magick++_ARCH_INCLUDE_DIR)
	include_directories(${ImageMagick_Magick++_ARCH_INCLUDE_DIR})
ENDIF()

# You shouldn't need to edit anything below this line 
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME} ${ImageMagick_Magick++_LIBRARY})

YURI_INSTALL_MODULE(${MODULE})
