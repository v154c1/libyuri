# Set name of the module
SET (MODULE event_info)

# Set all source files module uses
SET (SRC register.cpp
         EventDelay.cpp
         EventDelay.h
         EventInfo.cpp
		 EventInfo.h
		 EventTimer.cpp
		 EventTimer.h
		 EventToFrame.cpp
		 EventToFrame.h
		 EventValuePair.cpp
		 EventValuePair.h
		 PassEvent.cpp
		 PassEvent.h
		 EventConvolution.cpp
		 EventConvolution.h
		 )

IF(YURI_PLATFORM_LINUX)
SET(SRC ${SRC}
		 EventDevice.cpp
		 EventDevice.h
		 EventJoystick.cpp
		 EventJoystick.h)
ENDIF()

 
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME})

YURI_INSTALL_MODULE(${MODULE})
