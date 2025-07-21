//
// Created by neneko on 24.8.23.
//


#include "GLArrayBuffer.h"

namespace yuri {
    namespace gl {
        GLArrayBuffer::GLArrayBuffer(log::Log &log, size_t size) : log(log), size_(size) {
            glGenBuffers(1, &name_);
            bind();
            glBufferData(GL_ARRAY_BUFFER, size_ * sizeof(GLfloat), nullptr, GL_STATIC_DRAW);
            unbind();
        }

        void GLArrayBuffer::update(size_t start, const float *data, size_t data_size) {
            glBindBuffer(GL_ARRAY_BUFFER, name_);
            glBufferSubData(GL_ARRAY_BUFFER, start * sizeof(GLfloat), data_size * sizeof(GLfloat), data);
            unbind();
        }

        void GLArrayBuffer::bind() {
            glBindBuffer(GL_ARRAY_BUFFER, name_);
        }

        void GLArrayBuffer::unbind() {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }
}
