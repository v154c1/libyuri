# - Find the Newtek NDI includes and library
#
# This module searches libndi, the library for working with NDI streams.
#
# It defines the following variables
#  NDI_INCLUDE_DIRS, where to find Processing.NDI.Lib.h, etc.
#  NDI_LIBRARIES, the libraries to link against to use NDI.
#  NDI_FOUND, If false, do not try to use NDI.
#=============================================================================
# Copyright 2018 Jiri Melnikov, std.io
#
# based on FindPNG.cmake from cmake distribution 
# Copyright 2002-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

SET(NDI_INSTALL_DIR $ENV{NDI_DIRECTORY})
IF("${NDI_INSTALL_DIR}" STREQUAL "")
	# No NDI_DIRECTORY set, so let's try defalt installation path
	SET(NDI_INSTALL_DIR "/usr/local/ndi") 
ENDIF()
find_path(NDI_NDI_INCLUDE_DIR Processing.NDI.Lib.cplusplus.h
	${NDI_INSTALL_DIR}/include
)

  
find_library(NDI_NDI_LIBRARY ndi PATH ${NDI_INSTALL_DIR}/lib64)
  

if (NDI_NDI_LIBRARY AND NDI_NDI_INCLUDE_DIR)
	SET(NDI_INCLUDE_DIRS ${NDI_NDI_INCLUDE_DIR} )
	SET(NDI_INCLUDE_DIR ${NDI_NDI_INCLUDE_DIR} ) # for backward compatiblity
	SET(NDI_LIBRARY ${NDI_NDI_LIBRARY})
	SET(NDI_LIBRARIES ${NDI_NDI_LIBRARY})
	SET(NDI_VERSION "3.0")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NDI
                                  REQUIRED_VARS NDI_LIBRARY NDI_NDI_INCLUDE_DIR
                                  VERSION_VAR NDI_VERSION)

mark_as_advanced(NDI_NDI_INCLUDE_DIR NDI_NDI_LIBRARY)
