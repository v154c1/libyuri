//
// Created by neneko on 01.09.21.
//

#include "converters_all.h"


namespace yuri {
    namespace video {
        // declarations of get_converters_* methods
        converter_map get_converters_rgb();
        converter_map get_converters_rgb10bit();
        converter_map get_converters_single();
        converter_map get_converters_yuv();
        converter_map get_converters_yuv422();
        converter_map get_converters_yuv_rgb();

        namespace {
            void insert_partial_map(converter_map &conv, const converter_map &part) {
                conv.insert(part.cbegin(), part.cend());
            }

            converter_map get_all_converters() {
                converter_map conv;
                insert_partial_map(conv, get_converters_single());
                insert_partial_map(conv, get_converters_yuv());
                insert_partial_map(conv, get_converters_yuv422());
                insert_partial_map(conv, get_converters_yuv_rgb());
                insert_partial_map(conv, get_converters_rgb());
                insert_partial_map(conv, get_converters_rgb10bit());
                return conv;
            }


        }
        converter_map all_converters() {
            static converter_map conv = get_all_converters();
            return conv;
        }
    }
}