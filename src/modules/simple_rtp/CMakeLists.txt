# Set name of the module
SET (MODULE simple_rtp)

# Set all source files module uses
SET (SRC SimpleH264RtpSender.cpp
		 SimpleH264RtpSender.h
         SimpleH264RtpReceiver.cpp
         SimpleH264RtpReceiver.h
         SimpleH265RtpSender.cpp
		 SimpleH265RtpSender.h
         SimpleH265RtpReceiver.cpp
         SimpleH265RtpReceiver.h
         register.cpp)


 
add_library(${MODULE} MODULE ${SRC})
target_link_libraries(${MODULE} ${LIBNAME})

YURI_INSTALL_MODULE(${MODULE})
