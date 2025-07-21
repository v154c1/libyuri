# Set name of the module
SET (MODULE wl_window)

find_program(WAYLAND_SCANNER_EXEC wayland-scanner REQUIRED)

SET(XDG_SHELL_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c")
SET(XDG_SHELL_HEADER "${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h")

add_custom_command(
		OUTPUT "${XDG_SHELL_SOURCE}"
		COMMAND "${WAYLAND_SCANNER_EXEC}" "private-code"  "<" "/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml" ">" "${XDG_SHELL_SOURCE}"
		COMMENT "Generate XDG shell protocol"
)

add_custom_command(
		OUTPUT "${XDG_SHELL_HEADER}"
		COMMAND "${WAYLAND_SCANNER_EXEC}" "client-header"  "<" "/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml" ">" "${XDG_SHELL_HEADER}"
		COMMENT "Generate XDG shell header"
)

add_library(xdg_shell STATIC "${XDG_SHELL_SOURCE}" "${XDG_SHELL_HEADER}")

# Set all source files module uses
SET (SRC WlWindow.cpp
		 WlWindow.h)


 
add_library(${MODULE} MODULE ${SRC})
target_include_directories(${MODULE} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(${MODULE}  xdg_shell ${LIBNAME} ${WAYLAND_LIBRARIES} ${OPENGL_gl_LIBRARY} ${LIBNAME_HELPER_GL} EGL)

YURI_INSTALL_MODULE(${MODULE})
