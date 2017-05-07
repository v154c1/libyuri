# Set name of the module
SET (MODULE sdl_window)

# Set all source files module uses
SET (SRC SDLWindow.cpp
		 SDLWindow.h)

include_directories(${SDL_INCLUDE_DIR})
SET(LIBS ${LIBNAME} ${SDL_LIBRARY})

IF(NOT YURI_DISABLE_OPENGL)
    find_package(OpenGL)
    if (OPENGL_FOUND)
        include_directories(${OPENGL_INCLUDE_DIR})
        add_definitions("-DYURI_SDL_OPENGL=1")
        SET(LIBS ${LIBS} ${LIBNAME_HELPER_GL})
    ENDIF()
ENDIF()

add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE}  ${LIBS})

YURI_INSTALL_MODULE(${MODULE})
