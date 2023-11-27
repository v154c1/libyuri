/*!
 * @file 		HapDecoder.cpp
 * @author 		Zdenek Travnicek <v154c1@gmail.com>
 * @date 		04.09.2023
 * @copyright	Institute of Intermedia, CTU in Prague, 2023
 * 				Distributed under modified BSD Licence, details in file doc/LICENSE
 *
 */

#include "HapDecoder.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/compressed_frame_types.h"
#include "yuri/core/frame/CompressedVideoFrame.h"
#include "yuri/core/utils/irange.h"

#ifdef HAP_USE_SNAPPY

#include <snappy.h>
#include <snappy-sinksource.h>
#include <numeric>

#endif
namespace yuri {
    namespace hap_decoder {


        IOTHREAD_GENERATOR(HapDecoder)

        MODULE_REGISTRATION_BEGIN("hap_decoder")
            REGISTER_IOTHREAD("hap_decoder", HapDecoder)
        MODULE_REGISTRATION_END()

        core::Parameters HapDecoder::configure() {
            core::Parameters p = core::IOThread::configure();
            p.set_description("HapDecoder");
            return p;
        }


        HapDecoder::HapDecoder(const log::Log &log_, core::pwThreadBase parent, const core::Parameters &parameters) :
                core::IOFilter(log_, parent, std::string("hap_decoder")) {
            IOTHREAD_INIT(parameters)
        }

        HapDecoder::~HapDecoder() noexcept {
        }

        namespace {
            struct header_info_t {
                uint8_t code = 0;
                uint32_t size = 0;
                const uint8_t *data_start = nullptr;
            };


            header_info_t parse_header_data(const uint8_t *ptr, size_t size) {
                header_info_t head;
                if (size < 4) {
                    return {};
                }

                head.size = ptr[0] | ptr[1] << 8 | ptr[2] << 16;
                if (head.size == 0) {
                    if (size < 8) {
                        return {};
                    }
                    head.data_start = ptr + 8;
                    head.size = ptr[4] << 0 | ptr[5] << 8 | ptr[6] << 16 | ptr[7] << 24;
                } else {
                    head.data_start = ptr + 4;
                }
                head.code = ptr[3];
                return head;
            }

            template<class T>
            header_info_t parse_header_data(const T &data) {
                return parse_header_data(data.data(), data.size());
            }

            hap_info_t parse_header(log::Log &log, const core::pCompressedVideoFrame &cframe) {
                const auto &data = cframe->get_data();
                hap_info_t info;
                const auto head = parse_header_data(data);
                if (!head.data_start) {
                    return {};
                }
                info.data_start = head.data_start;
                info.size = head.size;
                uint8_t section_type = head.code;
                switch (section_type & 0xF0) {
                    case 0xA0:
                        info.compression = compression_t::none;
                        // no compression
                        break;
                    case 0xB0:
//                        info.snappy = true;
                        info.compression = compression_t::snappy;
                        break;
                    case 0xC0:
                        info.compression = compression_t::chunked;
                        break;
                    default:
                        log[log::warning] << "Unsupported compression 0x" << std::hex << (section_type & 0xF0);
                        return {};
                }
                switch (section_type & 0x0F) {
                    case 0x0B:
                        info.format = core::compressed_frame::dxt1;
                        break;
                    case 0x0E:
                        info.format = core::compressed_frame::dxt5;
                        break;
                    case 0x0F:
                        info.format = core::compressed_frame::ycocg_dxt5;
                        break;
                    default:
                        log[log::warning] << "Unsupported color type";
                        return {};
                }

                return info;
            }

            size_t dxt_size(format_t fmt, resolution_t resolution) {
                switch (fmt) {
                    case core::compressed_frame::dxt1:
                        return resolution.width * resolution.height / 2;
                    case core::compressed_frame::dxt5:
                    case core::compressed_frame::ycocg_dxt5:
                        return resolution.width * resolution.height;
                    default:
                        return 0;
                }

            }

#ifdef HAP_USE_SNAPPY

            uint32_t snappy_uncompressed_size(log::Log &log, const uint8_t *data, size_t size) {
                uint32_t uncompressed_size = 0;
                snappy::ByteArraySource src(reinterpret_cast<const char *>(data), size);
                if (!snappy::GetUncompressedLength(&src, &uncompressed_size)) {
                    log[log::error] << "Failed to get uncompressed size!";
                    return 0;
                }
                return uncompressed_size;
            }

#endif
        }

        core::pFrame HapDecoder::do_simple_single_step(core::pFrame frame) {
            if (frame->get_format() != core::compressed_frame::hap) {
                log[log::warning] << "Hap decoder needs a hap frame!";
                return {};
            }
            const auto cframe = std::dynamic_pointer_cast<core::CompressedVideoFrame>(frame);
            if (!cframe) {
                log[log::warning] << "Invalid frame frame!";
                return {};
            }
            const auto info = parse_header(log, cframe);
            if (info.format == core::compressed_frame::unknown) {
                log[log::warning] << "Failed to parse header!";
                return {};
            }

            switch (info.compression) {
                case compression_t::none:
                    return process_uncompressed_frame(cframe, std::move(info));
                case compression_t::snappy:
                    return process_snappy_frame(cframe, std::move(info));
                case compression_t::chunked:
                    return process_chunked_frame(cframe, std::move(info));
                default:
                    break;
            }

            log[log::error] << "Unsupported frame compression";
            return {};


        }

        core::pFrame
        HapDecoder::process_uncompressed_frame(const core::pCompressedVideoFrame &cframe, hap_info_t info) {
            switch (info.format) {
                case core::compressed_frame::dxt1:
                case core::compressed_frame::dxt5:
                case core::compressed_frame::ycocg_dxt5: {
                    const auto expected_size = dxt_size(info.format, cframe->get_resolution());
                    if (info.size != expected_size) {
                        log[log::warning] << "Wrong size! Expected " << expected_size << ", got " << info.size;
                        return {};
                    }
                    return core::CompressedVideoFrame::create_empty(info.format, cframe->get_resolution(),
                                                                    info.data_start, info.size);
                }
                default:
                    log[log::warning] << "Unsupported format!";
                    return {};
            }
        }

        core::pFrame
        HapDecoder::process_snappy_frame(const core::pCompressedVideoFrame &cframe, hap_info_t info) {
#ifndef HAP_USE_SNAPPY
            log[log::error] << "Compression supported not present!";
                        return {};
#else


            if (!snappy::IsValidCompressedBuffer(reinterpret_cast<const char *>(info.data_start), info.size)) {
                log[log::error] << "Data not valid snappy encoded frame!";
            }
            uint32_t uncompressed_size = snappy_uncompressed_size(log, info.data_start, info.size);
            if (uncompressed_size == 0) {
                return {};
            }
            const auto expected_size = dxt_size(info.format, cframe->get_resolution());
            if (uncompressed_size != expected_size) {
                log[log::warning] << "Wrong uncompressed size! Expected " << expected_size << ", got "
                                  << uncompressed_size;
                return {};
            }
            snappy::ByteArraySource src(reinterpret_cast<const char *>(info.data_start), info.size);
            switch (info.format) {
                case core::compressed_frame::dxt1:
                case core::compressed_frame::dxt5:
                case core::compressed_frame::ycocg_dxt5: {

                    auto out_frame = core::CompressedVideoFrame::create_empty(info.format, cframe->get_resolution(),
                                                                              uncompressed_size);
                    snappy::UncheckedByteArraySink sink(reinterpret_cast<char *>(&out_frame->get_data()[0]));
                    if (!snappy::Uncompress(&src, &sink)) {
                        log[log::warning] << "Failed to decompress data!";
                        return {};
                    }
                    return out_frame;
                }
                default:
                    log[log::warning] << "Unsupported format!";
                    return {};

            }
#endif
        }

        bool HapDecoder::set_param(const core::Parameter &param) {
            return core::IOThread::set_param(param);
        }


        core::pFrame
        HapDecoder::process_chunked_frame(const core::pCompressedVideoFrame &cframe, hap_info_t info) {
            // The data contain decoding instructions first

            const auto &head = parse_header_data(info.data_start, info.size);
            if (head.code != 1) {
                log[log::error] << "Missing decode instructions";
                return {};
            }
            auto tmphead = head;
            while (tmphead.size > 0) {
                const auto &subhead = parse_header_data(tmphead.data_start, tmphead.size);
                tmphead.size = std::distance(subhead.data_start + subhead.size, tmphead.data_start + tmphead.size);
                tmphead.data_start = subhead.data_start + subhead.size;
                switch (subhead.code) {
                    case 0x02:
                        log[log::verbose_debug] << "Found compressor table for " << subhead.size << " chunks";
                        info.chunks.resize(subhead.size);
                        for (auto i: irange(subhead.size)) {
                            switch (subhead.data_start[i]) {
                                case 0x0a:
                                    info.chunks[i].snappy = false;
                                    break;
                                case 0x0b:
                                    info.chunks[i].snappy = true;
                                    break;
                                default:
                                    log[log::error] << "Unsupported chunk compression 0x" << std::hex
                                                    << subhead.data_start[i];
                                    return {};
                            }
                        };
                        break;
                    case 0x03:
                        log[log::verbose_debug] << "Found chunk size table for " << subhead.size / 4 << " chunks";
                        info.chunks.resize(subhead.size / 4);
                        for (auto i: irange(subhead.size / 4)) {
                            const auto *ptr = subhead.data_start + (i * 4);
                            info.chunks[i].size = ptr[0] << 0 | ptr[1] << 8 | ptr[2] << 16 | ptr[3] << 24;
                        };
                        break;
                    case 0x04:
                        log[log::verbose_debug] << "Found chunk offset table for " << subhead.size / 4 << " chunks";
                        info.chunks.resize(subhead.size / 4);
                        for (auto i: irange(subhead.size / 4)) {
                            const auto *ptr = subhead.data_start + (i * 4);
                            info.chunks[i].offset = ptr[0] << 0 | ptr[1] << 8 | ptr[2] << 16 | ptr[3] << 24;
                        };
                        break;
                    default:
                        log[log::warning] << "Unsupported decode instruction section";
                }
            }

            const auto expected_size = dxt_size(info.format, cframe->get_resolution());

            info.size = std::distance(head.data_start + head.size, info.data_start + info.size);
            info.data_start = head.data_start + head.size;


            uint32_t offset = 0;

            for (auto &chunk: info.chunks) {
                if (chunk.offset == 0) {
                    chunk.offset = offset;
                }
                chunk.data_start = info.data_start + chunk.offset;
                offset += chunk.size;
                if (chunk.snappy) {
                    chunk.uncompressed_size = snappy_uncompressed_size(log, chunk.data_start, chunk.size);
                    if (chunk.uncompressed_size == 0) {
                        log[log::error] << "Failed to get chunk's uncompressed size! ";
                        return {};
                    }
                } else {
                    chunk.uncompressed_size = chunk.size;
                }
            }

            const size_t uncompressed_size = std::accumulate(info.chunks.cbegin(), info.chunks.cend(), size_t{0},
                                                             [](size_t acc, const chunk_t &chunk) {
                                                                 return acc + chunk.uncompressed_size;
                                                             });
            if (uncompressed_size != expected_size) {
                log[log::warning] << "Wrong uncompressed size! Expected " << expected_size << ", got "
                                  << uncompressed_size;
                return {};
            }

            switch (info.format) {
                case core::compressed_frame::dxt1:
                case core::compressed_frame::dxt5:
                case core::compressed_frame::ycocg_dxt5: {

                    auto out_frame = core::CompressedVideoFrame::create_empty(info.format, cframe->get_resolution(),
                                                                              uncompressed_size);
                    uint8_t *out_ptr = &out_frame->get_data()[0];
                    for (auto &chunk: info.chunks) {
                        if (chunk.snappy) {
                            snappy::ByteArraySource src(reinterpret_cast<const char *>(chunk.data_start), chunk.size);
                            snappy::UncheckedByteArraySink sink(
                                    reinterpret_cast<char *>(out_ptr));
                            if (!snappy::Uncompress(&src, &sink)) {
                                log[log::warning] << "Failed to decompress data!";
                                return {};
                            }
                        } else {
                            std::copy_n(chunk.data_start, chunk.uncompressed_size, out_ptr);
                        }
                        out_ptr += chunk.uncompressed_size;
                    }
                    return out_frame;
                }
                default:
                    log[log::error] << "Unsupported frame format!";
                    return {};
            }

        }
    }
    /* namespace hap_decoder */
} /* namespace yuri */
