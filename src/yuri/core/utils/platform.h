/*!
 * @file 		platform.h
 * @author 		Zdenek Travnicek
 * @date 		16.2.2013
 * @date		21.11.2013
 * @copyright	Institute of Intermedia, CTU in Prague, 2013
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 * @brief		Platform detection and platform specific defines
 */

#ifndef PLATFORM_H_
#define PLATFORM_H_

#if __cplusplus >= 201103L || _MSC_VER >= 1800
#ifndef YURI_USE_CXX11
#define YURI_USE_CXX11 1
#endif
#else
#error C++11 mode is required!
#endif


#if defined _WIN32
	#define YURI_WIN 1
	#define PACK_START			__pragma(pack(push,1))
	#define PACK_END			__pragma(pack(pop))
	#define DEPRECATED
	#ifdef yuri2_8_core_EXPORTS
		#define EXPORT			__declspec(dllexport)
		#define IMPORT			__declspec(dllimport)
	#else
		#define EXPORT 			__declspec(dllimport)
		#define IMPORT 			__declspec(dllexport)
	#endif
	#define MODULE_EXPORT			__declspec(dllexport)
	// Disable bad macros from windows.h
	#define WIN32_MEAN_AND_LEAN
	#define NOMINMAX
	// Disable warnings about "insecure" calls 
	#ifndef _SCL_SECURE_NO_WARNINGS
		#define _SCL_SECURE_NO_WARNINGS
	#endif
#elif defined __CYGWIN__
	#define YURI_CYGWIN 1
	#define YURI_POSIX 1
	#define EXPORT
	#define IMPORT
	#define MODULE_EXPORT
	#define PACK_START
	#define PACK_END			__attribute__((packed))
	#define DEPRECATED			__attribute__((deprecated))
	#if defined __CYGWIN__
		#define YURI_CYGWIN 1
	#endif
	#ifdef __STRICT_ANSI__
		#undef __STRICT_ANSI__
	#endif
#elif defined __linux__ 
	#define YURI_LINUX 1
	#define YURI_POSIX 1
	#define EXPORT
	#define IMPORT
	#define MODULE_EXPORT
	#define PACK_START
	#define PACK_END			__attribute__((packed))
	#define DEPRECATED			__attribute__((deprecated))
#elif defined __APPLE__
	#define YURI_APPLE 1
	#define YURI_POSIX 1
	#define EXPORT
	#define IMPORT	
	#define PACK_START
	#define PACK_END			__attribute__((packed))
	#define DEPRECATED			__attribute__((deprecated))
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
	#define YURI_BSD 1
	#define YURI_POSIX 1
	#define EXPORT
	#define IMPORT
	#define MODULE_EXPORT
	#define PACK_START
	#define PACK_END			__attribute__((packed))
	#define DEPRECATED			__attribute__((deprecated))
#else

	#error Unsupported platform
#endif

#ifdef __GNUG__
	#define GCC_VERSION (__GNUC__ * 10000 \
							   + __GNUC_MINOR__ * 100 \
							   + __GNUC_PATCHLEVEL__)
	#if GCC_VERSION < 40800
		// GCC 4.7.x doesn't support std::map::emplace ...
		#define EMPLACE_UNSUPPORTED  1
	#endif
	#if GCC_VERSION < 40801
		// GCC < 4.8.1 doesn't support ref qualified member functions...
		#define REF_QUALIFIED_MF_UNSUPPORTED  1
	#endif
#endif

#ifdef __clang__
	#ifdef __has_feature
		#if !__has_feature(cxx_reference_qualified_functions)
			#define REF_QUALIFIED_MF_UNSUPPORTED  1
		#endif
	#endif
#endif

#ifdef YURI_ANDROID

//#include <exception>
//namespace std {
//struct bad_cast : public exception {bad_cast operator()(){}};
//struct out_of_range: public exception {out_of_range operator()(){}};
//struct runtime_error: public exception {runtime_error operator()(){}};
//}
#endif


#if defined (__arm__) || defined(_M_ARM)
	#define YURI_ARCH_ARM 1
#elif defined( __x86_64__) || defined(_M_X64)
	#define YURI_ARCH_X86 1
	#define YURI_ARCH_X86_64 1
#elif defined(__i386__) || defined(_M_IX86)
	#define YURI_ARCH_X86 1
	#define YURI_ARCH_X86_32 1
#else
	#error Unsupported/untested architecture
#endif

#include "platform_hacks.h"

#endif /* PLATFORM_H_ */
