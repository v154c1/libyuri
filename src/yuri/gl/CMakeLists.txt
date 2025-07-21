SET(SRC  
	gl/GL.cpp
	gl/GL.h
	gl/GLProgram.cpp
	gl/GLProgram.h
	gl/GLShader.cpp
	gl/GLShader.h
	gl/GLArrayBuffer.cpp
	gl/GLArrayBuffer.h
	gl/GLVertexArray.cpp
	gl/GLVertexArray.h
	gl/default_shaders.h	
	)
	
IF(UNIX AND NOT YURI_DISABLE_GRABBER)
	SET(SRC ${SRC}
	gl/FBGrabber.cpp
	gl/FBGrabber.h
	)
ENDIF()
	
SET(YURI_HELPER_GL_SRC ${YURI_HELPER_GL_SRC}
	${SRC}
	PARENT_SCOPE)