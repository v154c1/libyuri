# Set name of the module
SET (MODULE glx_window)

# Set all source files module uses
SET (SRC GlxWindow.cpp
		 GlxWindow.h)


include_directories(${OPENGL_INCLUDE_DIR}) 
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME} ${X11_LIBRARIES} ${OPENGL_gl_LIBRARY} ${LIBNAME_HELPER_GL})

YURI_INSTALL_MODULE(${MODULE})
