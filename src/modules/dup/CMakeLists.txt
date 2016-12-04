# Set name of the module
SET (MODULE dup)

# Set all source files module uses
SET (SRC Dup.cpp
		 Dup.h)



# You shouldn't need to edit anything below this line 
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME})

YURI_INSTALL_MODULE(${MODULE})

IF (NOT YURI_DISABLE_TESTS)
	add_executable(module_dup_test dup_test.cpp  ${SRC})
	target_link_libraries (module_dup_test ${LIBNAME} ${LIBNAME_TEST})
	
	add_test (module_dup_test ${EXECUTABLE_OUTPUT_PATH}/module_dup_test)
ENDIF()