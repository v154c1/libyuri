# Set name of the module
SET(MODULE hap_decoder)

# Set all source files module uses
SET(SRC HapDecoder.cpp
        HapDecoder.h)

find_package(Snappy QUIET)
SET(DEPS "")


add_library(${MODULE} MODULE ${SRC})

if (Snappy_FOUND)
    SET(DEPS Snappy::snappy)
    target_compile_definitions(hap_decoder PRIVATE -DHAP_USE_SNAPPY)
endif ()


target_link_libraries(${MODULE} ${LIBNAME} ${DEPS})

YURI_INSTALL_MODULE(${MODULE})
