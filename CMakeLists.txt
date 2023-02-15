cmake_minimum_required (VERSION 3.0.0)
PROJECT(yuri2 VERSION 2.8.0 LANGUAGES CXX)
ENABLE_LANGUAGE(C)

SET(OLD_CMAKE FALSE)
IF (CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION EQUAL 8 AND CMAKE_PATCH_VERSION LESS 4)
	SET(OLD_CMAKE TRUE)
	MESSAGE("Old cmake version detected, disabling generator expressions")
ENDIF()
#################################################################
# Include required CMake modules
#################################################################
INCLUDE(FindPkgConfig)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckIncludeFileCXX)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/")
include(cuda_module)
include(GetGitRevisionDescription)
include(yuri_install)
#################################################################
# Get the version of this build
#################################################################
git_describe(YURI_GIT_VERSION)

IF("${YURI_GIT_VERSION}" STREQUAL "HEAD-HASH-NOTFOUND")
SET(YURI_GIT_VERSION "2.8.0")
ENDIF()
#################################################################
# Add strict warning if building under linux
#################################################################
SET(YURI_PLATFORM_LINUX FALSE)
SET(YURI_PLATFORM_APPLE FALSE)
SET(YURI_PLATFORM_BSD	FALSE)
SET(YURI_PLATFORM_WIN32 FALSE)
SET(YURI_PLATFORM_CYGWIN FALSE)


IF (UNIX OR MINGW OR CYGWIN)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra -std=c++0x -ffast-math")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -Wextra -std=gnu11 -ffast-math")
	SET(CMAKE_REQUIRED_FLAGS "${CMAKE_CXX_FLAGS}")
	IF("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
		MESSAGE(STATUS "Building for Linux")
		SET(YURI_PLATFORM_LINUX TRUE)
	ELSEIF("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD" OR "${CMAKE_SYSTEM_NAME}" STREQUAL "OpenBSD")
		SET(YURI_PLATFORM_BSD TRUE)
		MESSAGE(STATUS "Building for BSD")
	ELSEIF(APPLE)
		SET(YURI_PLATFORM_APPLE TRUE)
		MESSAGE(STATUS "Building for Apple")
	ELSEIF(CYGWIN)
		SET(YURI_PLATFORM_CYGWIN TRUE)
		MESSAGE(STATUS "Building for Cygwin")
	ELSE()
		MESSAGE(STATUS "Unknown platform: ${CMAKE_SYSTEM_NAME}, build may be incomplete")
	ENDIF()
	SET(CMAKE_REQUIRED_FLAGS "")
ELSEIF(WIN32)
	add_definitions( -DBOOST_ALL_NO_LIB )
	SET(YURI_PLATFORM_WIN32 TRUE)
	MESSAGE(STATUS "Building for Windows")
ELSE()
	MESSAGE(STATUS "Unknown platform: ${CMAKE_SYSTEM_NAME}, build may be incomplete")
ENDIF()

add_definitions("-DYURI_USE_CXX11") 
#################################################################
# Set versions
#################################################################
SET(YURI_CORE_VERSION 2.8)
SET(SO_VERSION 2.8.0)



#################################################################
# Setup names for the core library and helpers
#################################################################
SET(LIBNAME_PREFIX 			yuri${YURI_CORE_VERSION})
SET(LIBNAME 				${LIBNAME_PREFIX}_core)
SET(LIBNAME_HELPER_LIBAV 	${LIBNAME_PREFIX}_helper_av)
SET(LIBNAME_HELPER_CUDA 	${LIBNAME_PREFIX}_helper_cuda)
SET(LIBNAME_HELPER_GL 		${LIBNAME_PREFIX}_helper_gl)
SET(LIBNAME_TEST            yuri_test_main)

#################################################################
# Set prefix for modules
#################################################################
SET (CMAKE_SHARED_MODULE_PREFIX "yuri2.8_module_")

#################################################################
# Setup paths
#################################################################
include_directories( ${CMAKE_SOURCE_DIR}/src )
SET (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
SET (MODULE_DIR ${EXECUTABLE_OUTPUT_PATH}/modules)
SET (TARGET_MODULE_PATH lib/yuri2/)
IF(WIN32)
	SET (ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
	SET (RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
	SET (LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
	SET (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
ENDIF()
#################################################################
# Initially disable all helpers
#################################################################
SET(CORE_LIBAV FALSE)
SET(CORE_CUDA FALSE)
SET(CORE_X11 FALSE)
SET(CORE_GL FALSE)

#################################################################
# Add some options to control build
#################################################################
OPTION (BUILD_SHARED_LIBS "Build shared libraries." ON)
OPTION (YURI_DISABLE_CUDA "Disable building of CUDA helpers" ON)
OPTION (YURI_DISABLE_LIBAV "Disable building of libav helpers and support" OFF)
OPTION (YURI_DISABLE_SAGE "Disable building of libsage related modules" ON)
OPTION (YURI_DISABLE_X11 "Explicitly disable building of X11 helpers and related modules" ON)
OPTION (YURI_DISABLE_OPENGL "Explicitly disable building of opengl helpers and related modules" OFF)
OPTION (YURI_DISABLE_IEEE1394 "Disable building of IEEE1394 sources" OFF)
OPTION (YURI_DISABLE_DECKLINK "Disable building of decklink API helpers" OFF)
OPTION (YURI_DISABLE_PNG "Disable building of PNG module" OFF)
OPTION (YURI_DISABLE_BOOST "Disable boost dependencies" OFF)
OPTION (YURI_DISABLE_GPUJPEG  "Disable GPUJPEG library" ON)
OPTION (YURI_DISABLE_OPENCV "Disable OpenCV modules" OFF)
OPTION (YURI_DISABLE_ULTRAGRID "Disable ultragrid helper and modules" ON)

OPTION (YURI_DISABLE_GRABBER "Disable node fb_grabber in GL helper" OFF)

OPTION (YURI_BUILD_UNSTABLE_MODULES "Enable building of unstable modules" ON)
OPTION (YURI_BUILD_EXPERIMENTAL_MODULES "Enable building of experimental modules" ON)

OPTION (YURI_DISABLE_TESTS "Disable unit tests" ON )

#################################################################
# Conditionaly enable testing
#################################################################

IF( NOT YURI_DISABLE_TESTS )
	enable_testing()
ENDIF()

#################################################################
# Test for missing std functions
#################################################################

INCLUDE (CheckCXXSourceCompiles)

SET(TEST_FOR_TO_STRING "#include <string>
int main() {
	auto x = std::to_string(5);
}
")

SET(TEST_FOR_TO_STOI "#include <string>
int main() {
	auto x = std::stoi(\"5\");
}
")

SET(TEST_FOR_TO_STOL "#include <string>
int main() {
	auto x = std::stol(\"5\");
}
")

SET(TEST_FOR_TO_STOUL "#include <string>
int main() {
	auto x = std::stoul(\"5\");
}
")

CHECK_CXX_SOURCE_COMPILES("${TEST_FOR_TO_STRING}" HAVE_TO_STRING)
CHECK_CXX_SOURCE_COMPILES("${TEST_FOR_TO_STOI}" HAVE_STOI)
CHECK_CXX_SOURCE_COMPILES("${TEST_FOR_TO_STOL}" HAVE_STOL)
CHECK_CXX_SOURCE_COMPILES("${TEST_FOR_TO_STOUL}" HAVE_STOUL)

IF(NOT HAVE_TO_STRING)
	MESSAGE(STATUS "Platform is missing std::to_string(), compiling replacement")
	add_definitions("-DYURI_STD_TO_STRING_MISSING")
ENDIF()

IF(NOT HAVE_STOI)
	MESSAGE(STATUS "Platform is missing std::stoi(), compiling replacement")
	add_definitions("-DYURI_STD_STOI_MISSING")
ENDIF()

IF(NOT HAVE_STOL)
	MESSAGE(STATUS "Platform is missing std::stol(), compiling replacement")
	add_definitions("-DYURI_STD_STOL_MISSING")
ENDIF()

IF(NOT HAVE_STOUL)
	MESSAGE(STATUS "Platform is missing std::stoul(), compiling replacement")
	add_definitions("-DYURI_STD_STOUL_MISSING")
ENDIF()


#################################################################
# Fix warning for CMP0072 (OpenGL_GL_PREFERENCE)
#################################################################

if(POLICY CMP0072)
	cmake_policy(SET CMP0072 NEW)
endif()

#################################################################
# And finally submerge into source directories
#################################################################
add_subdirectory(src)
add_subdirectory(doc)


#################################################################
# Specify defaults for packaging into a debian package
#################################################################

SET(CPACK_GENERATOR "DEB")
SET(CPACK_PACKAGE_VENDOR "st.dio s.r.o.")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "<zdenek@stdio.cz>>")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-all-dev, libgl1-mesa-dev, libavcodec-dev, libavformat-dev, libavutil-dev")
SET(CPACK_PACKAGE_CHECKSUM SHA256)

# Simple hack to get detailed version
if(NOT GIT_FOUND)
	find_package(Git QUIET)
endif()

SET(GIT_COMMIT_COUNT "")
if (GIT_FOUND)
	execute_process(COMMAND
			${GIT_EXECUTABLE}
			describe
			WORKING_DIRECTORY
			"${CMAKE_SOURCE_DIR}"
			RESULT_VARIABLE
			res
			OUTPUT_VARIABLE
			out
			ERROR_QUIET
			OUTPUT_STRIP_TRAILING_WHITESPACE)
	if(res EQUAL 0)
		string(REPLACE "-" ";" out_list ${out})
		list(GET out_list 1 com)
		SET(GIT_COMMIT_COUNT "${com}")
	endif()
ENDIF()

find_program(LSB_RELEASE_EXEC lsb_release)
if(LSB_RELEASE_EXEC)
    execute_process(COMMAND ${LSB_RELEASE_EXEC} -rs OUTPUT_VARIABLE LSB_RELEASE OUTPUT_STRIP_TRAILING_WHITESPACE )
    execute_process(COMMAND ${LSB_RELEASE_EXEC} -is OUTPUT_VARIABLE LSB_DIST OUTPUT_STRIP_TRAILING_WHITESPACE )
    SET(SYSTEM_SUFFIX "-${LSB_DIST}-${LSB_RELEASE}")
else()
    SET(SYSTEM_SUFFIX "")
endif()

SET(CPACK_DEBIAN_PACKAGE_RELEASE "${GIT_COMMIT_COUNT}")
SET(CPACK_DEBIAN_FILE_NAME "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${GIT_COMMIT_COUNT}${SYSTEM_SUFFIX}-Linux.deb")
INCLUDE(CPack)