#pragma once

#include <glad/glad.h>

#ifdef OPENGL_DEBUG
#include <spdlog/spdlog.h>
#define GL_CALL(_CALL)                                                          \
    do {                                                                        \
        _CALL;                                                                  \
        GLenum gl_err = glGetError();                                           \
        if (gl_err != 0) {                                                      \
            SPDLOG_ERROR("GL error {} returned from '{}'", gl_err, #_CALL);    \
        }                                                                       \
    } while (0) // Call with error check
#else
#define GL_CALL(_CALL) _CALL // Call without error check
#endif

namespace GL {

bool CheckShader(GLuint handle, const char* desc);
bool CheckProgram(GLuint handle, const char* desc);
GLuint CreateShader(const GLchar* vertexShader, const GLchar* fragmentShader);

} // namespace GL