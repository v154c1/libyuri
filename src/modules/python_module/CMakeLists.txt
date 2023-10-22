# Set name of the module
SET(MODULE python_module)

# Set all source files module uses
SET(SRC PythonModule.cpp
        PythonModule.h
        python_wrappers.h)


add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME} Boost::python Python3::Python)

YURI_INSTALL_MODULE(${MODULE})
