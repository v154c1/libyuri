#################################################################
# Core library can use boost filessytem 
#################################################################
IF(NOT YURI_DISABLE_BOOST)
	set( Boost_USE_STATIC_LIBS OFF )
	find_package( Boost 1.49 COMPONENTS filesystem system regex)
	IF(Boost_FILESYSTEM_FOUND)
		add_definitions(-DHAVE_BOOST_FILESYSTEM)
	ENDIF()
	IF(Boost_REGEX_FOUND)
		add_definitions(-DHAVE_BOOST_REGEX)		
	ENDIF()
ENDIF()
#################################################################
# Checking for optional dependencies for helper libraries
#################################################################

IF(NOT YURI_DISABLE_X11)
	find_package( X11 )		
ENDIF()
IF (NOT YURI_DISABLE_GL)
    find_package( OpenGL )
ENDIF() 
IF(NOT YURI_DISABLE_LIBAV)
	pkg_check_modules(libavcodec libavcodec)
ENDIF()
IF(NOT YURI_DISABLE_CUDA)
	find_package( CUDA)
ENDIF()

CHECK_INCLUDE_FILE_CXX (stdint.h HAVE_STDINT_H)

IF (NOT HAVE_STDINT_H)
	MESSAGE(FATAL_ERROR "Missing stdint.h. Please update your compile chain")
ENDIF()


#################################################################
# Set up dependencies for libyuri_core
#################################################################
IF(Boost_SYSTEM_FOUND)
    SET (YURI_LINK ${YURI_LINK} ${Boost_LIBRARY_DIRS} )
    #IF(NOT WIN32)
    SET (YURI_LIBS ${YURI_LIBS}	${Boost_LIBRARIES} )
    #ENDIF()
    SET (YURI_INCLUDE ${YURI_INCLUDE} ${Boost_INCLUDE_DIRS} )  
ENDIF()
#################################################################
# Set up dependencies for helper libraries
#################################################################
SET (YURI_HELPER_LIBAV_LINK ${YURI_HELPER_LINK} ${libavcodec_LIBRARY_DIRS})
		
SET (YURI_HELPER_LIBAV_LIBS ${YURI_HELPER_LIBAV_LIBS} 
		${LIBNAME} ${libavcodec_LIBRARIES})

SET (YURI_HELPER_LIBAV_INCLUDE  ${YURI_HELPER_LIBAV_INCLUDE}  ${libavcodec_INCLUDE_DIRS})

IF(NOT YURI_DISABLE_CUDA AND CUDA_FOUND)
	SET(YURI_HELPER_CUDA_INCLUDE ${YURI_HELPER_CUDA_INCLUDE} ${CUDA_INCLUDE_DIRS})
	SET(YURI_HELPER_CUDA_LIBS ${LIBNAME} ${YURI_HELPER_CUDA_LIBS} ${CUDA_LIBRARIES})
	SET(YURI_HELPER_CUDA_LINK ${YURI_HELPER_CUDA_LINK} ${CUDA_LIBRARY_DIRS})
ENDIF()

IF(NOT YURI_DISABLE_OPENGL AND OPENGL_FOUND)
	SET(YURI_HELPER_GL_LIBS ${LIBNAME} ${YURI_HELPER_OPENGL_LIBS} )
	IF (OPENGL_gl_LIBRARY)
		SET(YURI_HELPER_GL_LIBS ${YURI_HELPER_GL_LIBS} ${OPENGL_gl_LIBRARY})
	ENDIF()
	IF (OPENGL_glu_LIBRARY) 
		SET(YURI_HELPER_GL_LIBS ${YURI_HELPER_GL_LIBS} ${OPENGL_glu_LIBRARY})
	ENDIF()
	IF (OPENGL_glx_LIBRARY) 
		SET(YURI_HELPER_GL_LIBS ${YURI_HELPER_GL_LIBS} ${OPENGL_glx_LIBRARY})
	ENDIF()

ENDIF()
#################################################################
# Adding CXXFLAGS and LDFLAGS for libyuri_core
#################################################################
IF(WIN32)
add_definitions("-DYURI_MODULE_IN_TREE")
else()
add_definitions("-DYURI_MODULE_IN_TREE -DINSTALL_PREFIX=\\\"${CMAKE_INSTALL_PREFIX}\\\"")
endif()
link_directories ( ${YURI_LINK} ${YURI_HELPER_LIBAV_LINK} ${YURI_HELPER_X11_LINK})
include_directories ( ${YURI_INCLUDE} ${YURI_HELPER_LIBAV_INCLUDE} ${YURI_HELPER_X11_INCLUDE} ${OPENGL_INCLUDE_DIR})

#################################################################
# Submerge into subdirectories to collect source files
#################################################################
add_subdirectory(log)
add_subdirectory(exception)
add_subdirectory(core)
add_subdirectory(event)
#################################################################
# Collect sources for helper libraries
#################################################################
IF(NOT YURI_DISABLE_LIBAV AND libavcodec_FOUND)
	add_subdirectory(libav)
	SET(CORE_LIBAV TRUE PARENT_SCOPE)
	SET(CORE_LIBAV TRUE)
	add_definitions("-DYURI_HAVE_LIBAV")
ELSE()
	SET(CORE_LIBAV FALSE PARENT_SCOPE)
	SET(CORE_LIBAV FALSE)
ENDIF()
IF(NOT YURI_DISABLE_CUDA AND CUDA_FOUND)
	add_subdirectory(cuda)
	SET(CORE_CUDA TRUE PARENT_SCOPE)
	SET(CORE_CUDA TRUE)
	add_definitions("-DYURI_HAVE_CUDA")
	SET(CUDA_PROPAGATE_HOST_FLAGS OFF)
	list(APPEND CUDA_NVCC_FLAGS "-std=c++11;-O2")
	CUDA_INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src )
ELSE()
	SET(CORE_CUDA FALSE PARENT_SCOPE)
	SET(CORE_CUDA FALSE)
ENDIF()
IF(NOT YURI_DISABLE_OPENGL AND OPENGL_FOUND)
	add_subdirectory(gl)
	SET(CORE_GL TRUE PARENT_SCOPE)
	SET(CORE_GL TRUE)
	add_definitions("-DYURI_HAVE_GL")
ELSE()
	SET(CORE_GL FALSE PARENT_SCOPE)
	SET(CORE_GL FALSE)
ENDIF()

#################################################################
# Generate version.cpp with current git revision
#################################################################
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/version.cpp" @ONLY)
SET(YURI_SRC ${YURI_SRC} "${CMAKE_CURRENT_BINARY_DIR}/version.cpp" ${CMAKE_CURRENT_SOURCE_DIR}/version.h )

#################################################################
# Add target for libyuri_core
#################################################################
add_library(${LIBNAME} ${YURI_SRC})

#################################################################
# Add targets for helper libraries
#################################################################
SET(LIBDIR "${CMAKE_INSTALL_LIBDIR}")
IF(NOT LIBDIR)
	SET(LIBDIR "lib")
ENDIF()

IF(${CORE_CUDA})
	MESSAGE(STATUS "Building CUDA helper library!")
	IF(${CORE_X11})
		SET(YURI_HELPER_CUDA_LIBS ${YURI_HELPER_CUDA_LIBS} ${LIBNAME_HELPER_X11})
	ENDIF()
	cuda_add_library(${LIBNAME_HELPER_CUDA} ${YURI_HELPER_CUDA_SRC})
	target_link_libraries ( ${LIBNAME_HELPER_CUDA} ${YURI_HELPER_CUDA_LIBS})
	SET_TARGET_PROPERTIES( ${LIBNAME_HELPER_CUDA} PROPERTIES SOVERSION ${SO_VERSION} VERSION ${YURI_CORE_VERSION} )
	install(TARGETS ${LIBNAME_HELPER_CUDA} 	RUNTIME DESTINATION bin
						LIBRARY DESTINATION ${LIBDIR}
						ARCHIVE DESTINATION ${LIBDIR})
	
ENDIF()

IF(${CORE_LIBAV})
	MESSAGE(STATUS "Building libav helper library!")
	add_library(${LIBNAME_HELPER_LIBAV} ${YURI_HELPER_LIBAV_SRC})
	target_link_libraries ( ${LIBNAME_HELPER_LIBAV} ${YURI_HELPER_LIBAV_LIBS})
	SET_TARGET_PROPERTIES( ${LIBNAME_HELPER_LIBAV} PROPERTIES SOVERSION ${SO_VERSION} VERSION ${YURI_CORE_VERSION} )
	install(TARGETS ${LIBNAME_HELPER_LIBAV} 	RUNTIME DESTINATION bin
						LIBRARY DESTINATION ${LIBDIR}
						ARCHIVE DESTINATION ${LIBDIR})
	
ENDIF()

IF(${CORE_GL})
	MESSAGE(STATUS "Building OpenGL helper library!")
	add_library(${LIBNAME_HELPER_GL} ${YURI_HELPER_GL_SRC})
	target_link_libraries ( ${LIBNAME_HELPER_GL} ${YURI_HELPER_GL_LIBS})
	SET_TARGET_PROPERTIES( ${LIBNAME_HELPER_GL} PROPERTIES SOVERSION ${SO_VERSION} VERSION ${YURI_CORE_VERSION} )
	install(TARGETS ${LIBNAME_HELPER_GL} 	RUNTIME DESTINATION bin
						LIBRARY DESTINATION ${LIBDIR} 
						ARCHIVE DESTINATION ${LIBDIR})
ENDIF()

IF(UNIX)
	IF(APPLE)
		SET( YURI_LIBS ${YURI_LIBS} dl)
	ELSEIF(YURI_PLATFORM_LINUX)
		SET( YURI_LIBS ${YURI_LIBS} dl rt pthread)
	ELSEIF(YURI_PLATFORM_BSD)
		SET( YURI_LIBS ${YURI_LIBS})
	ELSE()
		SET( YURI_LIBS ${YURI_LIBS} rt )
	ENDIF()
ENDIF()
#################################################################
# Set linking for libyuri_core
#################################################################
target_link_libraries ( ${LIBNAME} ${YURI_LIBS})
MESSAGE("lib dir ${LIBDIR}")
SET_TARGET_PROPERTIES( ${LIBNAME} PROPERTIES SOVERSION ${SO_VERSION} VERSION ${YURI_CORE_VERSION} )
install(TARGETS ${LIBNAME} 	RUNTIME DESTINATION bin
						LIBRARY DESTINATION ${LIBDIR} 
						ARCHIVE DESTINATION ${LIBDIR})
install(DIRECTORY ../yuri DESTINATION include
          				FILES_MATCHING PATTERN "*.h")
               

						
