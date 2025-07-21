//
// Created by neneko on 24.8.23.
//

#ifndef YURI2_GLVERTEXARRAY_H
#define YURI2_GLVERTEXARRAY_H

#include "GLArrayBuffer.h"

namespace yuri {
    namespace gl {
        class GLVertexArray {
        public:
            GLVertexArray(log::Log &log);

            //            void update(size_t start, const std::vector<float> &data) {
            //                update(start, data.data(), data.size());
            //            }
            //
            //            void update(size_t start, const float *data, size_t data_size);
            //
            void bind();

            void unbind();

            void update_coords(float *data, size_t size);

            void update_tex(float *data, size_t size);

        private:
            log::Log &log;
            GLuint name_;
            GLArrayBuffer coord_buffer_;
            GLArrayBuffer tex_buffer_;
            //            size_t size_;
        };
    }
}
#endif //YURI2_GLVERTEXARRAY_H
