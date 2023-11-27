/*!
 * @file 		HapDecoder.h
 * @author 		Zdenek Travnicek <v154c1@gmail.com>
 * @date 		04.09.2023
 * @copyright	Institute of Intermedia, CTU in Prague, 2023
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#ifndef HAPDECODER_H_
#define HAPDECODER_H_

#include "yuri/core/thread/IOFilter.h"
#include "yuri/core/thread/ConverterThread.h"
#include "yuri/core/frame/compressed_frame_types.h"

namespace yuri {
    namespace hap_decoder {

        enum class compression_t {
            unknown,
            none,
            snappy,
            chunked
        };
        struct chunk_t {
            bool snappy = false;
            size_t size = 0;
            size_t offset = 0;
            size_t uncompressed_size = 0;
            const uint8_t* data_start = nullptr;
        };
        struct hap_info_t {
            format_t format = core::compressed_frame::unknown;
            size_t size = 0;
            const uint8_t *data_start = nullptr;
//                bool snappy = false;
//                bool chunked = false;
            compression_t compression = compression_t::unknown;
            std::vector<chunk_t> chunks;

        };

        class HapDecoder : public core::IOFilter {
        public:
            IOTHREAD_GENERATOR_DECLARATION

            static core::Parameters configure();

            HapDecoder(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters);

            virtual ~HapDecoder() noexcept;

        private:
            virtual core::pFrame do_simple_single_step(core::pFrame frame) override;

            virtual bool set_param(const core::Parameter &param) override;


            core::pFrame process_uncompressed_frame(const core::pCompressedVideoFrame &cframe,
                                                     yuri::hap_decoder::hap_info_t info);

            core::pFrame
            process_snappy_frame(const core::pCompressedVideoFrame &cframe, yuri::hap_decoder::hap_info_t info);


            core::pFrame
            process_chunked_frame(const core::pCompressedVideoFrame &cframe, yuri::hap_decoder::hap_info_t info);
        };

    } /* namespace hap_decoder */
} /* namespace yuri */
#endif /* HAPDECODER_H_ */
