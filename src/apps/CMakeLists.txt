

SET(LIBS ${LIBNAME})

IF(NOT YURI_DISABLE_BOOST)
	find_package( Boost 1.46 COMPONENTS program_options regex)
	IF(Boost_PROGRAM_OPTIONS_FOUND)
		add_definitions(-DHAVE_BOOST_PROGRAM_OPTIONS)
        SET(LIBS ${LIBS} ${Boost_LIBRARIES})
		add_definitions(-DBOOST_PROGRAM_OPTIONS_DYN_LINK)
	ENDIF()
	
	IF(Boost_FOUND)
		include_directories ( ${Boost_INCLUDE_DIRS})
		link_directories (${Boost_LIBRARY_DIRS})
	ENDIF()

ENDIF()


add_executable(yuri2 	yuri/yuri_listings.cpp 
						yuri/yuri_listings.h
						yuri/try_conversion.cpp
						yuri/try_conversion.h
						yuri2.cpp)


target_link_libraries ( yuri2 ${LIBS})

install(TARGETS yuri2 RUNTIME DESTINATION bin)

add_executable(yuri_test_module test_module.cpp)
target_link_libraries (yuri_test_module ${LIBNAME} )
install(TARGETS yuri_test_module RUNTIME DESTINATION bin)

IF(UNIX AND NOT YURI_DISABLE_GRABBER AND NOT YURI_DISABLE_OPENGL)
find_package(OpenGL)
	IF (OPENGL_FOUND)
	include_directories(${OPENGL_INCLUDE_DIR}) 
	add_library(yuri_wrapper MODULE 
			wrapper.cpp
			wrapper/WrapperLoader.h
			wrapper/WrapperLoader.cpp
			)
	
	target_link_libraries (yuri_wrapper ${LIBNAME} ${LIBNAME_HELPER_GL} )
	YURI_INSTALL_EXE_LIB(yuri_wrapper)
	ENDIF()
ENDIF()

IF(Boost_REGEX_FOUND)
add_executable(yuri_simple 	
						yuri_simple.cpp
						simple/SimpleBuilder.h
						simple/SimpleBuilder.cpp)
target_link_libraries ( yuri_simple ${LIBNAME} ${Boost_REGEX_LIBRARY})
install(TARGETS yuri_simple RUNTIME DESTINATION bin)
ENDIF()
