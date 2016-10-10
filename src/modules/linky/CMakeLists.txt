# Set name of the module
SET (MODULE linky)

# Set all source files module uses
SET (SRC linky_common.cpp
         linky_common.h 
         LinkyOutput.cpp
		 LinkyOutput.h
		 LinkyInput.cpp
         LinkyInput.h
         jsoncpp.cpp
         json_helpers.h
         json/json.h
         json/json-forwards.h
         register.cpp)


 
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME} ${CURL_LIBRARIES})

YURI_INSTALL_MODULE(${MODULE})
