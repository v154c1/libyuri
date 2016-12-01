/*!
 * @file 		RenderText.cpp
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		29.01.2015
 * @copyright	Institute of Intermedia, 2015
 * 				Distributed BSD License
 *
 */

#include "RenderText.h"
#include "yuri/core/Module.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/core/frame/raw_frame_types.h"
#include "yuri/core/frame/raw_frame_params.h"
#include "yuri/core/utils/irange.h"
#include "yuri/core/utils/assign_events.h"
#include "yuri/core/utils/string_generator.h"
#include "yuri/core/utils/utf8.h"
namespace yuri {
namespace freetype {

IOTHREAD_GENERATOR(RenderText)

MODULE_REGISTRATION_BEGIN("freetype")
REGISTER_IOTHREAD("render_text", RenderText)
MODULE_REGISTRATION_END()

core::Parameters RenderText::configure()
{
    core::Parameters p = base_type::configure();
    p.set_description("RenderText");
    p["font"]["Path to font file"]                                                                       = "/usr/share/fonts/corefonts/arial.ttf";
    p["font2"]["Path to fallback font file"]                                                             = "";
    p["text"]["Text"]                                                                                    = "Hello world";
    p["size"]["Font size in pixels"]                                                                     = 64;
    p["resolution"]["Image resolution for generated image"]                                              = resolution_t{ 800, 600 };
    p["position"]["Text position"]                                                                       = coordinates_t{ 0, 0 };
    p["line_height"]["Height of a line (used only for multiline text). If 0, font_size is used instead"] = 0;
    p["spacing"]["Additional spacing between characters"]                                                = 0;
    p["generate"]["Generate blank image. Set to false to put text on incomming images"]                  = false;
    p["kerning"]["Enable/disable kerning."]                                                              = true;
    p["fps"]["Framerate for generated images. Set to 0 to generate only on text change"]                 = 0.0;
    p["blend"]["Blend edged - nicer output, but slower"]                                                 = true;
    p["utf8"]["Handle text as utf8"]                                                                     = true;
    p["color"]["Text color"]                                                                             = core::color_t::create_rgb(0xFF, 0xFF, 0xFF);
    return p;
}

RenderText::RenderText(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters)
    : base_type(log_, parent, std::string("freetype")),
      BasicEventConsumer(log),
      resolution_({ 800, 600 }),
      position_({ 0, 0 }),
      char_spacing_{ 0 },
      generate_{ false },
      edge_blend_{ true },
      modified_{ true },
      utf8_{ true },
      color_(core::color_t::create_rgb(0xFF, 0xFF, 0xFF))
{
    set_latency(50_ms);
    IOTHREAD_INIT(parameters)
    FT_Init_FreeType(&library_);
    if (FT_New_Face(library_, font_file_.c_str(), 0, &face_)) {
        throw exception::InitializationFailed("Failed to load font face");
    }
    log[log::info] << "Loaded font: " << face_->family_name << ", " << face_->style_name;
    FT_Set_Pixel_Sizes(face_, font_size_, 0);
    if (!font_file2_.empty()) {
        if (FT_New_Face(library_, font_file2_.c_str(), 0, &face2_)) {
            log[log::warning] << "Failed to load fallback font";
            font_file2_.clear();
        } else {
            log[log::info] << "Loaded fallback font: " << face2_->family_name << ", " << face2_->style_name;
            FT_Set_Pixel_Sizes(face2_, font_size_, 0);
        }
    }

    if (kerning_ && !FT_HAS_KERNING(face_)) {
        log[log::warning] << "Kerning requested but not supported by current font...";
    }
    using namespace core::raw_format;
    set_supported_formats({ y8, rgb24, bgr24, rgba32, bgra32, argb32, abgr32, yuv444, yuyv422, yvyu422, uyvy422, vyuy422, yuva4444 });
}

RenderText::~RenderText() noexcept
{
}

namespace {
using namespace core::raw_format;

template <bool blend>
struct compute_value;

template <>
struct compute_value<true> {
    template <typename T, typename T2, typename T3>
    static auto eval(T p, T2 out, T3 color) -> typename std::remove_reference<decltype(*out)>::type
    {
        using out_type = typename std::remove_reference<decltype(*out)>::type;

        const auto max_p = std::numeric_limits<T>::max();

        const auto val = (static_cast<int64_t>(*out) * (max_p - p)) + (static_cast<int64_t>(color * p));
        return static_cast<out_type>(val / max_p);
    }
};

template <>
struct compute_value<false> {
    template <typename T, typename T2, typename T3>
    static auto eval(T p, T2 out, T3 color) -> typename std::remove_reference<decltype(*out)>::type
    {
        (void)out;
        return (static_cast<int64_t>(p) * color) / std::numeric_limits<T>::max();
    }
};

template <format_t fmt, bool blend>
struct draw_kernel;

template <bool blend>
struct draw_kernel<y8, blend> {
    template <typename T, typename T2>
    static void draw(T in, const T in_end, T2 out, const std::array<uint8_t, 4>& color, bool)
    {
        while (in != in_end) {
            const auto p = *in;
            if (p)
                *out = compute_value<blend>::eval(p, out, color[0]);
            ++in;
            ++out;
        }
    }
};

template <bool blend>
struct draw_kernel<rgb24, blend> {
    template <typename T, typename T2>
    static void draw(T in, const T in_end, T2 out, const std::array<uint8_t, 4>& color, bool)
    {
        while (in != in_end) {
            const auto p = *in;
            if (p) {
                *(out + 0) = compute_value<blend>::eval(p, out + 0, color[0]);
                *(out + 1) = compute_value<blend>::eval(p, out + 1, color[1]);
                *(out + 2) = compute_value<blend>::eval(p, out + 2, color[2]);
            }
            ++in;
            out += 3;
        }
    }
};

template <bool blend>
struct draw_kernel<bgr24, blend> {
    template <typename T, typename T2>
    static void draw(T in, const T in_end, T2 out, const std::array<uint8_t, 4>& color, bool)
    {
        draw_kernel<rgb24, blend>::draw(in, in_end, out, { { color[2], color[1], color[0], color[3] } }, true);
    }
};

template <bool blend>
struct draw_kernel<yuv444, blend> {
    template <typename T, typename T2>
    static void draw(T in, const T in_end, T2 out, const std::array<uint8_t, 4>& color, bool)
    {
        while (in != in_end) {
            const auto p = *in;
            if (p) {
                *(out + 0) = compute_value<blend>::eval(p, out + 0, color[0]);
                *(out + 1) = compute_value<blend>::eval(p, out + 1, color[1]);
                *(out + 2) = compute_value<blend>::eval(p, out + 2, color[2]);
            }
            ++in;
            out += 3;
        }
    }
};

template <bool blend>
struct draw_kernel<rgba32, blend> {
    template <typename T, typename T2>
    static void draw(T in, const T in_end, T2 out, const std::array<uint8_t, 4>& color, bool)
    {
        const auto out_max = std::numeric_limits<typename std::remove_reference<decltype(*out)>::type>::max();
        while (in != in_end) {
            const auto p = *in;
            if (p) {
                *(out + 0) = compute_value<blend>::eval(p, out + 0, color[0]);
                *(out + 1) = compute_value<blend>::eval(p, out + 1, color[1]);
                *(out + 2) = compute_value<blend>::eval(p, out + 2, color[2]);
                *(out + 3) = out_max;
            }
            ++in;
            out += 4;
        }
    }
};

template <bool blend>
struct draw_kernel<bgra32, blend> {
    template <typename T, typename T2>
    static void draw(T in, const T in_end, T2 out, const std::array<uint8_t, 4>& color, bool)
    {
        draw_kernel<rgba32, blend>::draw(in, in_end, out, { { color[2], color[1], color[0], color[3] } }, true);
    }
};

template <bool blend>
struct draw_kernel<argb32, blend> {
    template <typename T, typename T2>
    static void draw(T in, const T in_end, T2 out, const std::array<uint8_t, 4>& color, bool)
    {
        const auto out_max = std::numeric_limits<typename std::remove_reference<decltype(*out)>::type>::max();
        while (in != in_end) {
            const auto p = *in;
            if (p) {
                *(out + 0) = out_max;
                *(out + 1) = compute_value<blend>::eval(p, out + 1, color[0]);
                *(out + 2) = compute_value<blend>::eval(p, out + 2, color[1]);
                *(out + 3) = compute_value<blend>::eval(p, out + 3, color[2]);
            }
            ++in;
            out += 4;
        }
    }
};

template <bool blend>
struct draw_kernel<abgr32, blend> {
    template <typename T, typename T2>
    static void draw(T in, const T in_end, T2 out, const std::array<uint8_t, 4>& color, bool)
    {
        draw_kernel<argb32, blend>::draw(in, in_end, out, { { color[2], color[1], color[0], color[3] } }, true);
    }
};

template <format_t fmt, bool blend>
void draw_glyph_impl(core::pRawVideoFrame frame, const FT_Bitmap& bmp, coordinates_t position, geometry_t draw_rect, const std::array<uint8_t, 4>& color)
{
    auto       data     = PLANE_RAW_DATA(frame, 0);
    const auto linesize = PLANE_DATA(frame, 0).get_line_size();
    const auto bpp      = core::raw_format::get_fmt_bpp(frame->get_format(), 0) >> 3;
    for (auto y : irange(draw_rect.y, draw_rect.y + draw_rect.height)) {
        auto           data_out        = data + linesize * y + draw_rect.x * bpp;
        const auto     bmp_line_offset = y - position.y;
        const auto     bmp_col_offset  = draw_rect.x - position.x;
        const uint8_t* data_in         = &bmp.buffer[bmp.pitch * bmp_line_offset + bmp_col_offset];
        const auto     data_in_end     = data_in + draw_rect.width;
        draw_kernel<fmt, blend>::draw(data_in, data_in_end, data_out, color, position.x % 2 == 1);
    }
}

template <bool blend>
struct draw_kernel<yuyv422, blend> {
    template <typename T, typename T2>
    static void draw(T in, const T in_end, T2 out, const std::array<uint8_t, 4>& color, bool swap)
    {
        if (swap)
            draw(in, in_end, out, { { color[0], color[2], color[1], color[3] } }, false);
        else {
            while (in < (in_end - 1)) {
                const auto p = *in;
                ++in;
                const auto p2 = *in;
                ++in;
                if (p)
                    *out = compute_value<blend>::eval(p, out, color[0]);
                ++out;
                if (p)
                    *out = compute_value<blend>::eval(p, out, color[1]);
                ++out;
                if (p2)
                    *out = compute_value<blend>::eval(p2, out, color[0]);
                ++out;
                if (p2)
                    *out = compute_value<blend>::eval(p2, out, color[2]);
                ++out;
            }
            if (in < in_end) {
                const auto p = *in;
                ++in;
                if (p)
                    *out = compute_value<blend>::eval(p, out, color[0]);
                ++out;
                if (p)
                    *out = compute_value<blend>::eval(p, out, color[1]);
                ++out;
            }
        }
    }
};

template <bool blend>
struct draw_kernel<yvyu422, blend> {
    template <typename T, typename T2>
    static void draw(T in, const T in_end, T2 out, const std::array<uint8_t, 4>& color, bool swap)
    {
        draw_kernel<yuyv422, blend>::draw(in, in_end, out, color, !swap);
    }
};

template <bool blend>
struct draw_kernel<uyvy422, blend> {
    template <typename T, typename T2>
    static void draw(T in, const T in_end, T2 out, const std::array<uint8_t, 4>& color, bool swap)
    {
        if (swap)
            draw(in, in_end, out, { { color[0], color[2], color[1], color[3] } }, false);
        else {
            while (in < in_end - 1) {

                const auto p = *in;
                ++in;
                if (p)
                    *out = compute_value<blend>::eval(p, out, color[1]);
                ++out;
                if (p)
                    *out = compute_value<blend>::eval(p, out, color[0]);
                ++out;

                const auto p2 = *in;
                ++in;
                if (p2)
                    *out = compute_value<blend>::eval(p2, out, color[2]);
                ++out;
                if (p2)
                    *out = compute_value<blend>::eval(p2, out, color[0]);
                ++out;
            }
            if (in < in_end) {
                const auto p = *in;
                ++in;
                if (p)
                    *out = compute_value<blend>::eval(p, out, color[1]);
                ++out;
                if (p)
                    *out = compute_value<blend>::eval(p, out, color[0]);
                ++out;
            }
        }
    }
};

template <bool blend>
struct draw_kernel<vyuy422, blend> {
    template <typename T, typename T2>
    static void draw(T in, const T in_end, T2 out, const std::array<uint8_t, 4>& color, bool swap)
    {
        draw_kernel<uyvy422, blend>::draw(in, in_end, out, color, !swap);
    }
};

template <bool blend>
struct draw_kernel<yuva4444, blend> {
    template <typename T, typename T2>
    static void draw(T in, const T in_end, T2 out, const std::array<uint8_t, 4>& color, bool)
    {
        const auto out_max = std::numeric_limits<typename std::remove_reference<decltype(*out)>::type>::max();
        while (in != in_end) {
            const auto p = *in;
            if (p) {
                *(out + 0) = compute_value<blend>::eval(p, out + 0, color[0]);
                *(out + 1) = compute_value<blend>::eval(p, out + 1, color[1]);
                *(out + 2) = compute_value<blend>::eval(p, out + 2, color[2]);
                *(out + 3) = out_max;
            }
            ++in;
            out += 4;
        }
    }
};

template <format_t fmt>
void draw_glyph_impl(core::pRawVideoFrame frame, const FT_Bitmap& bmp, coordinates_t position, geometry_t draw_rect, bool blend,
                     const std::array<uint8_t, 4>& color)
{
    if (blend)
        draw_glyph_impl<fmt, true>(frame, bmp, position, draw_rect, color);
    else
        draw_glyph_impl<fmt, false>(frame, bmp, position, draw_rect, color);
}

bool draw_glyph(FT_GlyphSlot glyph, core::pRawVideoFrame frame, coordinates_t position, bool blend, const core::color_t& color)
{
    const auto& bitmap       = glyph->bitmap;
    const auto  bmp_geometry = geometry_t{ static_cast<dimension_t>(bitmap.width), static_cast<dimension_t>(bitmap.rows), position.x, position.y };

    const auto frame_resolution = frame->get_resolution();

    const auto draw_rect = intersection(frame_resolution, bmp_geometry);
    if (!draw_rect)
        return false;
    switch (frame->get_format()) {
    case y8:
        draw_glyph_impl<y8>(frame, bitmap, position, draw_rect, blend, color.get_yuva());
        break;
    case rgb24:
        draw_glyph_impl<rgb24>(frame, bitmap, position, draw_rect, blend, color.get_rgba());
        break;
    case bgr24:
        draw_glyph_impl<bgr24>(frame, bitmap, position, draw_rect, blend, color.get_rgba());
        break;
    case rgba32:
        draw_glyph_impl<rgba32>(frame, bitmap, position, draw_rect, blend, color.get_rgba());
        break;
    case bgra32:
        draw_glyph_impl<bgra32>(frame, bitmap, position, draw_rect, blend, color.get_rgba());
        break;
    case argb32:
        draw_glyph_impl<argb32>(frame, bitmap, position, draw_rect, blend, color.get_rgba());
        break;
    case abgr32:
        draw_glyph_impl<abgr32>(frame, bitmap, position, draw_rect, blend, color.get_rgba());
        break;
    case yuv444:
        draw_glyph_impl<yuv444>(frame, bitmap, position, draw_rect, blend, color.get_yuva());
        break;
    case yuyv422:
        draw_glyph_impl<yuyv422>(frame, bitmap, position, draw_rect, blend, color.get_yuva());
        break;
    case yvyu422:
        draw_glyph_impl<yvyu422>(frame, bitmap, position, draw_rect, blend, color.get_yuva());
        break;
    case uyvy422:
        draw_glyph_impl<uyvy422>(frame, bitmap, position, draw_rect, blend, color.get_yuva());
        break;
    case vyuy422:
        draw_glyph_impl<vyuy422>(frame, bitmap, position, draw_rect, blend, color.get_yuva());
        break;
    case yuva4444:
        draw_glyph_impl<yuva4444>(frame, bitmap, position, draw_rect, blend, color.get_yuva());
        break;
    default:
        return false;
    }

    return true;
}
}

void RenderText::run()
{
    core::pRawVideoFrame gframe;
    if (generate_) {
        // Create an empty frame for generate(
        gframe = core::RawVideoFrame::create_empty(core::raw_format::y8, resolution_);
        std::fill(PLANE_DATA(gframe, 0).begin(), PLANE_DATA(gframe, 0).end(), 0);
    }
    Timer timer;
    bool  fps_valid   = fps_ != 0.0;
    auto  frame_delta = fps_valid ? 1_s / fps_ : 0_s;

    while (still_running()) {
        if (!generate_) {
            wait_for(get_latency());
            process_events();
            step();
        } else {
            process_events();
            if (modified_ || (fps_valid && timer.get_duration() > frame_delta)) {
                timer.reset();
                push_frame(0, do_special_single_step(gframe));
                modified_ = false;
            } else {
                if (fps_valid)
                    sleep((frame_delta - timer.get_duration()) / 2.0);
                else
                    sleep(get_latency());
            }
            //			draw_text(text_, frame);
            //			push_frame(0, frame);
        }
    }
}

core::pFrame RenderText::do_special_single_step(core::pRawVideoFrame frame)
{
    auto f    = get_frame_unique(frame);
    auto text = core::utils::generate_string(text_, 0, frame);
    draw_text(text, f);
    return f;
}

void RenderText::draw_text(const std::string& text, core::pRawVideoFrame& frame)
{
    coordinates_t position          = position_;
    double        horiz_pos         = 0.0;
    char32_t      prev              = 0;
    bool          do_kerning        = kerning_ && FT_HAS_KERNING(face_);
    char32_t      unicode_character = 0;
    int           remaining         = 0;
    bool          backslash         = false;
    for (auto c : text) {
        if (utf8_) {
            std::tie(unicode_character, remaining) = utils::utf8_char(c, unicode_character, remaining);
        } else {
            unicode_character = c & 0xFF;
        }
        if (remaining > 0)
            continue;
        if (unicode_character == '\\') {
            if (!backslash) {
                backslash = true;
                continue;
            }
        }

        if (backslash) {
            backslash = false;
            switch (unicode_character) {
            case 'n':
                horiz_pos = 0;
                position.y += line_height_ ? line_height_ : font_size_;
                continue;
            case 'r':
                horiz_pos = 0;
                continue;
            default:
                break;
            }
        }

        FT_GlyphSlot glyph = nullptr;
        auto         idx   = FT_Get_Char_Index(face_, unicode_character);
        if (idx == 0 && !font_file2_.empty()) {
            idx = FT_Get_Char_Index(face2_, unicode_character);
            if (idx == 0) {
                log[log::debug] << "Unsupported character found";
            }
            FT_Load_Glyph(face2_, idx, FT_LOAD_RENDER);
            glyph = face2_->glyph;
        } else {
            FT_Load_Glyph(face_, idx, FT_LOAD_RENDER);
            glyph = face_->glyph;
        }

        if (do_kerning) {
            uint32_t idx = FT_Get_Char_Index(face_, unicode_character);
            if (prev) {
                FT_Vector delta;
                FT_Get_Kerning(face_, prev, idx, FT_KERNING_UNFITTED, &delta);
                const double kern = (delta.x >> 6) + static_cast<double>(delta.x & 0x3F) / 0x3F;
                horiz_pos += kern;
            }
            prev = idx;
        }
        auto coord = coordinates_t{ static_cast<position_t>(horiz_pos) + glyph->bitmap_left, -glyph->bitmap_top } + position;
        draw_glyph(glyph, frame, coord, edge_blend_ && !generate_, color_);
        const double advance = ((glyph->linearHoriAdvance & 0xFFFF0000) >> 16) + static_cast<double>(glyph->linearHoriAdvance & 0xFFFF) / 0xFFFF;
        horiz_pos += advance;
    }
}

bool RenderText::set_param(const core::Parameter& param)
{
    if (assign_parameters(param)      //
        (font_file_, "font")          //
        (font_file2_, "font2")        //
        (text_, "text")               //
        (font_size_, "size")          //
        (position_, "position")       //
        (resolution_, "resolution")   //
        (char_spacing_, "spacing")    //
        (line_height_, "line_height") //
        (generate_, "generate")       //
        (kerning_, "kerning")         //
        (edge_blend_, "blend")        //
        (fps_, "fps")                 //
        (utf8_, "utf8")               //
        (color_, "color"))
        return true;
    return base_type::set_param(param);
}

bool RenderText::do_process_event(const std::string& event_name, const event::pBasicEvent& event)
{
    if (assign_events(event_name, event) //
        (text_, "text", "message")       //
        (color_, "color")) {
        modified_ = true;
        return true;
    }
    if (iequals(event_name, "delete")) {
        text_     = "";
        modified_ = true;
        return true;
    }
    if (assign_events(event_name, event) //
        (position_, "position")          //
        (position_.x, "x")               //
        (position_.y, "y"))
        return true;
    return false;
}
} /* namespace freetype */
} /* namespace yuri */
