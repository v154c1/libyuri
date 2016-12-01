# Set name of the module
SET (MODULE webserver)

# Set all source files module uses
SET (SRC WebServer.cpp
		 WebServer.h
		 WebPageGenerator.cpp
		 WebPageGenerator.h
		 base64.cpp
		 base64.h
		 urlencode.cpp
		 urlencode.h
		 WebResource.cpp
		 WebResource.h
		 WebImageResource.cpp
		 WebImageResource.h
		 WebStaticResource.cpp
		 WebStaticResource.h
		 WebControlResource.cpp
		 WebControlResource.h
		 WebDirectoryResource.cpp
		 WebDirectoryResource.h
		 web_exceptions.h
		 register.cpp
		 )


 
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME} ${Boost_REGEX_LIBRARY})

YURI_INSTALL_MODULE(${MODULE})


IF (NOT YURI_DISABLE_TESTS)
    add_executable(module_webserver_test test_main.cpp test_encoding.cpp base64.cpp urlencode.cpp  )
    target_link_libraries (module_webserver_test ${LIBNAME} )
    
    add_test (module_webserver_test ${EXECUTABLE_OUTPUT_PATH}/module_webserver_test)
ENDIF()