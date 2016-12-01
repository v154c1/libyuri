/*!
 * @file 		RenderText.h
 * @author 		Zdenek Travnicek <travnicek@iim.cz>
 * @date		29.01.2015
 * @copyright	Institute of Intermedia, 2015
 * 				Distributed BSD License
 *
 */

#ifndef RENDERTEXT_H_
#define RENDERTEXT_H_

#include "yuri/core/thread/SpecializedIOFilter.h"
#include "yuri/core/frame/RawVideoFrame.h"
#include "yuri/event/BasicEventConsumer.h"
#include "yuri/core/utils/color.h"
#include <ft2build.h>
//#include <freetype/freetype.h>
#include FT_FREETYPE_H

namespace yuri {
namespace freetype {

class RenderText : public core::SpecializedIOFilter<core::RawVideoFrame>, public event::BasicEventConsumer {
    using base_type = core::SpecializedIOFilter<core::RawVideoFrame>;

public:
    IOTHREAD_GENERATOR_DECLARATION
    static core::Parameters configure();
    RenderText(const log::Log& log_, core::pwThreadBase parent, const core::Parameters& parameters);
    virtual ~RenderText() noexcept;

private:
    virtual void run() override;

    virtual core::pFrame do_special_single_step(core::pRawVideoFrame frame) override;
    virtual bool set_param(const core::Parameter& param) override;
    virtual bool do_process_event(const std::string& event_name, const event::pBasicEvent& event) override;
    void draw_text(const std::string& text, core::pRawVideoFrame& frame);

private:
    FT_Library library_;
    FT_Face    face_;
    FT_Face    face2_;

    std::string   font_file_;
    std::string   font_file2_;
    size_t        font_size_;
    std::string   text_;
    resolution_t  resolution_;
    coordinates_t position_;
    position_t    char_spacing_;
    position_t    line_height_;
    bool          generate_;
    bool          kerning_;
    double        fps_;
    bool          edge_blend_;
    bool          modified_;
    bool          utf8_;
    core::color_t color_;
};

} /* namespace freetype */
} /* namespace yuri */
#endif /* RENDERTEXT_H_ */
