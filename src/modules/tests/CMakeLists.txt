add_executable(module_generic_test test_modules.cpp  )
target_link_libraries (module_generic_test ${LIBNAME} ${LIBNAME_TEST})

add_test (module_generic_test ${EXECUTABLE_OUTPUT_PATH}/module_generic_test WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/..)