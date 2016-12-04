# Set name of the module
SET (MODULE osc_module)

# Set all source files module uses
SET (SRC OSCReceiver.cpp
		 OSCReceiver.h
		 OSCSender.cpp
		 OSCSender.h
		 OSC.h
		 register.cpp)


 
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME})

YURI_INSTALL_MODULE(${MODULE})

IF (NOT YURI_DISABLE_TESTS)
	add_executable(module_osc_test osc_test.cpp  )
	target_link_libraries (module_osc_test ${LIBNAME} ${LIBNAME_TEST})
	
	add_test (module_osc_test ${EXECUTABLE_OUTPUT_PATH}/module_osc_test)
ENDIF()