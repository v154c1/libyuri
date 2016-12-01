SET(YURI_SRC ${YURI_SRC} 
	core/frame/Frame.cpp core/frame/Frame.h
	core/frame/VideoFrame.cpp core/frame/VideoFrame.h
	core/frame/RawVideoFrame.cpp core/frame/RawVideoFrame.h
	core/frame/CompressedVideoFrame.cpp core/frame/CompressedVideoFrame.h
	core/frame/AudioFrame.cpp core/frame/AudioFrame.h
	core/frame/RawAudioFrame.cpp core/frame/RawAudioFrame.h
	core/frame/EventFrame.cpp core/frame/EventFrame.h 
	core/frame/raw_frame_params.cpp core/frame/raw_frame_params.h
	core/frame/raw_frame_types.h
	core/frame/raw_frame_traits.h
	core/frame/compressed_frame_types.h
	core/frame/compressed_frame_params.cpp core/frame/compressed_frame_params.h
	core/frame/raw_audio_frame_params.cpp core/frame/raw_audio_frame_params.h
	core/frame/raw_audio_frame_types.h
	
	core/utils/Timer.cpp core/utils/Timer.h
	
	core/pipe/Pipe.cpp core/pipe/Pipe.h
	core/pipe/PipePolicies.cpp core/pipe/PipePolicies.h
	core/pipe/PipeGenerator.h core/pipe/PipeGenerator.cpp
	core/pipe/SpecialPipes.cpp core/pipe/SpecialPipes.h
	core/pipe/PipeNotification.cpp core/pipe/PipeNotification.h
	
	core/utils/Singleton.h
	core/utils/BasicGenerator.h
	core/utils/ModuleLoader.cpp core/utils/ModuleLoader.h
	core/utils/DirectoryBrowser.cpp core/utils/DirectoryBrowser.h
	core/utils/MultiRegister.h
	core/utils/SingleRegister.h
	core/utils/StateTransitionTable.h
	core/utils/irange.h
	core/utils/make_list.h
	core/utils/trace_method.h
	core/utils/hostname.cpp core/utils/hostname.h
	core/utils/frame_info.cpp core/utils/frame_info.h
	core/utils/global_time.cpp core/utils/global_time.h
	core/utils/string_generator.cpp core/utils/string_generator.h
	core/utils/managed_resource.h
	core/utils/wall_time.cpp core/utils/wall_time.h
	core/utils/environment.cpp core/utils/environment.h
	core/utils/string.h
	core/utils/color.cpp core/utils/color.h
	core/utils/color_events.cpp
	core/utils/any.h
	core/utils/utf8.h
	
	core/thread/builder_utils.cpp
	core/thread/builder_utils.h
	core/thread/GenericBuilder.cpp
	core/thread/GenericBuilder.h
	core/thread/XmlBuilder.cpp
	core/thread/XmlBuilder.h
	
	core/thread/IOThread.cpp core/thread/IOThread.h
	core/thread/IOThreadGenerator.cpp core/thread/IOThreadGenerator.h
	core/thread/IOFilter.cpp core/thread/IOFilter.h
	core/thread/MultiIOFilter.cpp core/thread/MultiIOFilter.h

	core/thread/PipeConnector.cpp core/thread/PipeConnector.h
	core/thread/ThreadBase.cpp core/thread/ThreadBase.h
	core/thread/ThreadChild.cpp core/thread/ThreadChild.h
	core/thread/ThreadSpawn.cpp core/thread/ThreadSpawn.h
	core/thread/FixedMemoryAllocator.cpp core/thread/FixedMemoryAllocator.h

	core/thread/ConverterThread.cpp core/thread/ConverterThread.h
	core/thread/ConvertUtils.cpp core/thread/ConvertUtils.h
	core/thread/ConverterRegister.cpp core/thread/ConverterRegister.h
	core/thread/Convert.cpp core/thread/Convert.h
	
	core/thread/InputThread.h core/thread/InputThread.cpp
	core/thread/InputRegister.h core/thread/InputRegister.cpp
	
	core/parameter/Parameters.cpp core/parameter/Parameters.h
	core/parameter/Parameter.h
	
	core/socket/socket_errors.h
	core/socket/DatagramSocket.cpp core/socket/DatagramSocket.h
	core/socket/StreamSocket.cpp core/socket/StreamSocket.h
	core/socket/DatagramSocketGenerator.cpp core/socket/DatagramSocketGenerator.h
	core/socket/StreamSocketGenerator.cpp core/socket/StreamSocketGenerator.h
	core/socket/NullSockets.cpp core/socket/NullSockets.h
	

#	core/BasicFrame.cpp
#	core/BasicFrame.h
#	core/BasicIOMacros.h
#	core/BasicIOFilter.cpp
#	core/BasicIOFilter.h
#	core/BasicPipe.cpp
#	core/BasicPipe.h
#	core/PipeConnector.cpp
#	core/PipeConnector.h
#	core/pipe_types.h
#	core/SocketBase.cpp
#	core/SocketBase.h
#	core/types.h
#	core/utils.h
	
#	core/Callback.cpp
#	core/Callback.h
#	core/config_common.h

#	core/ConfigException.cpp
#	core/ConfigException.h
	
#core/thread/RegisteredClass.cpp
#core/thread/RegisteredClass.h
#	core/Instance.cpp
#	core/Instance.h
#	core/Parameters.cpp
#	core/Parameters.h
 
	core/tinyxml/tinystr.cpp
	core/tinyxml/tinystr.h
	core/tinyxml/tinyxml.cpp
	core/tinyxml/tinyxmlerror.cpp
	core/tinyxml/tinyxml.h
	core/tinyxml/tinyxmlparser.cpp
	
	
	PARENT_SCOPE)
	
	#core/Config.cpp
	#core/Config.h
	
	
add_executable(yuri_core_test test.cpp)
target_link_libraries ( yuri_core_test ${LIBNAME})