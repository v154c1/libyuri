//
// Created by neneko on 24.8.23.
//

#ifndef YURI2_GLARRAYBUFFER_H
#define YURI2_GLARRAYBUFFER_H
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <GL/gl.h>
//#include <string>
#include "yuri/log/Log.h"
#include <vector>

namespace yuri {
    namespace gl {
        class GLArrayBuffer {
        public:
            GLArrayBuffer(log::Log &log, size_t size);

            void update(size_t start, const std::vector<float> &data) {
                update(start, data.data(), data.size());
            }

            void update(size_t start, const float *data, size_t data_size);

            void bind();

            void unbind();

        private:
            log::Log &log;
            GLuint name_;
            size_t size_;
        };
    }
}

#endif //YURI2_GLARRAYBUFFER_H
