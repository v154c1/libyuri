//
// Created by neneko on 30.1.23.
//

#ifndef YURI2_LOGUTILS_H
#define YURI2_LOGUTILS_H
#include <string>
#include <optional>
#include <map>

namespace yuri {
    namespace log {


/**
 * @brief Flags representing debug levels and flags
 */
        enum debug_flags_t: long {
            silent 			=	0,
            fatal 			=	1,			//!< Fatal error, application will probably quit
            error	 		=	2,			//!< Error, application will recover, but some data or state may be lost
            warning	 		=	3,			//!< Warning, application should continue (but may not work correctly)
            info			=	4,			//!< Information about execution
            debug 			=	5,			//!< Debug information not needed during normal usage
            verbose_debug	=	6,			//!< Verbose debug, used for debugging only
            trace			=	7,			//!< Tracing information, should never be needed during normal work
            flag_mask		=   7,			//!< Mask representing all flags
            show_time		=	1L << 4,	//!< Enables output of actual time with the output
            show_thread_id	=	1L << 5,	//!< Enables showing thread id
            show_level		=	1L << 6,	//!< Enables showing debug level name
            use_colors		=	1L << 7,	//!< Enable usage of colors
            show_date		=	1L << 8,	//!< Enables output of actual date with the output
        };

        typedef debug_flags_t  debug_flags;


        struct LogMessageInfo {
            LogMessageInfo(int uid, debug_flags level, long output_flags, const std::string& logger_name): uid{uid},level{level},output_flags{output_flags},logger_name{logger_name}{}
            int uid;
            debug_flags level;
            long output_flags;
            const std::string& logger_name;
            std::string time;
        };

        extern const std::map<debug_flags_t, std::string> level_names;
        extern const std::map<debug_flags_t, std::string> level_colors;


        template<class Stream>
        void print_level(Stream& os, debug_flags flags, const long output_flags)
        {
            if (!(output_flags & show_level)) {
                return;
            }
            const auto f = static_cast<debug_flags>(flags&flag_mask);
            const auto it = level_names.find(f);
            if (it == level_names.end()) {
                return;
            }
            const auto& name = it->second;

            if (output_flags&use_colors) {
                auto it_col = level_colors.find(f);
                auto it_col2 = level_colors.find(info);
                if (it_col!=level_colors.end() && it_col2 != level_colors.end()) {
                    const auto& color1 = it_col->second;
                    const auto& color2 = it_col2->second;
                    os <<  color1 << name << color2 << " ";
                    return;
                }
            }
            os << name << " ";
        }

    }
}
#endif //YURI2_LOGUTILS_H
