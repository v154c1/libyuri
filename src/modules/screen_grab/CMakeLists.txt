# Set name of the module
SET (MODULE screen_grab)

# Set all source files module uses
SET (SRC ScreenGrab.cpp
		 ScreenGrab.h)



# You shouldn't need to edit anything below this line
include_directories(${X11_Xlib_INCLUDE_PATH}) 
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME} ${X11_LIBRARIES} ${X11_XFIXES_LIBRARIES})

YURI_INSTALL_MODULE(${MODULE})
