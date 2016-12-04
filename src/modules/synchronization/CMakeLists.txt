# Set name of the module
SET (MODULE synchronization)
 
# Set all source files module uses
SET (ONEPC_SRC
    OnepcProtocolCohort.cpp
    OnepcProtocolCohort.h
    OnepcProtocolCoordinator.cpp
    OnepcProtocolCoordinator.h)

SET (TWOPC_SRC
    TwopcProtocol.h
	TwopcProtocolCoordinator.cpp
	TwopcProtocolCoordinator.h
	TwopcProtocolCohort.cpp
	TwopcProtocolCohort.h
	TwopcTimeoutProtocolCoordinator.cpp
	TwopcTimeoutProtocolCoordinator.h
	TwopcTimeoutProtocolCohort.cpp
	TwopcTimeoutProtocolCohort.h
	)


SET (SRC 
    DelayEstimation.cpp
    DelayEstimation.h
    TimestampObserver.cpp
	TimestampObserver.h
	
	${ONEPC_SRC}
	
	${TWOPC_SRC}
	
	PlaybackController.cpp
	PlaybackController.h
	
    register.cpp)
 
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME})
 
YURI_INSTALL_MODULE(${MODULE})


IF (NOT YURI_DISABLE_TESTS)
	add_executable(module_onepc_protocol_test onepc_protocol_test.cpp  ${ONEPC_SRC} )
	target_link_libraries (module_onepc_protocol_test ${LIBNAME} ${LIBNAME_TEST})
	add_test (module_onepc_protocol_test ${EXECUTABLE_OUTPUT_PATH}/module_onepc_protocol_test)
	
	add_executable(module_twopc_protocol_test twopc_protocol_test.cpp  )
	target_link_libraries (module_twopc_protocol_test ${LIBNAME} ${LIBNAME_TEST})
	add_test (module_twopc_protocol_test ${EXECUTABLE_OUTPUT_PATH}/module_twopc_protocol_test)
ENDIF()
