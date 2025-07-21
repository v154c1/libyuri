//
// Created by neneko on 24.8.23.
//

#include "GLVertexArray.h"

namespace yuri {
    namespace gl {
        GLVertexArray::GLVertexArray(log::Log &log) : log(log), coord_buffer_(log, 12), tex_buffer_(log, 24) {
            glGenVertexArrays(1, &name_);
            bind();
            glEnableVertexAttribArray(0);
            coord_buffer_.bind();
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(1);
            tex_buffer_.bind();
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
            unbind();
        }

        void GLVertexArray::bind() {
            //            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(name_);
        }

        void GLVertexArray::update_coords(float *data, size_t size) {
            if (size != 8) {
                log[log::error] << "Update coords require 8 values!";
                return;
            }
            coord_buffer_.update(0, data, 6);
            coord_buffer_.update(6, data + 4, 4);
            coord_buffer_.update(10, data, 2);
        }

        void GLVertexArray::update_tex(float *data, size_t size) {
            if (size != 16) {
                log[log::error] << "Tex coords require 16 values!";
                return;
            }
            tex_buffer_.update(0, data, 12);
            tex_buffer_.update(12, data + 8, 8);
            tex_buffer_.update(20, data, 4);
        }

        void GLVertexArray::unbind() {
            glBindVertexArray(0);
        }
    }
}
