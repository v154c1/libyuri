//
// Created by neneko on 30.1.23.
//

#include <map>
#include "LogUtils.h"
#include "yuri/core/utils/platform.h"
namespace yuri {
    namespace log {

            const std::map<debug_flags_t, std::string> level_names = {
                    {fatal,         "FATAL ERROR"},
                    {error,         "ERROR"},
                    {warning,       "WARNING"},
                    {info,          "INFO"},
                    {debug,         "DEBUG"},
                    {verbose_debug, "VERBOSE_DEBUG"},
                    {trace,         "TRACE"}};

            const std::map<debug_flags_t, std::string> level_colors = {
#if defined(YURI_LINUX) || defined(YURI_BSD)
                    {fatal,			"\033[4;31;42m"}, // Red, underscore, bg
    {error,			"\033[31m"}, // Red
    {warning,		"\033[35m"},
    {info,			"\033[00m"},
    {debug,			"\033[35m"},
    {verbose_debug,	"\033[36m"},
    {trace,			"\033[4;36m"}
#else
                    {fatal, ""},
                    {error, ""},
                    {warning, ""},
                    {info, ""},
                    {debug, ""},
                    {verbose_debug, ""},
                    {trace, ""}
#endif
            };

        }

}