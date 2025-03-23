#include "framework.h"

#include <cstdio>
#include <string>

bool GL::CheckShader(GLuint handle, const char* desc)
{
    GLint status = 0, logLength = 0;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLength);
    if ((GLboolean)status == GL_FALSE) {
        SPDLOG_ERROR("failed to compile: {}!", desc);
    }
    if (logLength > 1) {
        std::string buf;
        buf.resize((int)(logLength + 1));
        glGetShaderInfoLog(handle, logLength, nullptr, (GLchar*)buf.data());
        SPDLOG_ERROR(buf);
    }
    return (GLboolean)status == GL_TRUE;
}

bool GL::CheckProgram(GLuint handle, const char* desc)
{
    GLint status = 0, logLength = 0;
    glGetProgramiv(handle, GL_LINK_STATUS, &status);
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &logLength);
    if ((GLboolean)status == GL_FALSE) {
        SPDLOG_ERROR("failed to link: {}!", desc);
    }
    if (logLength > 1) {
        std::string buf;
        buf.resize((int)(logLength + 1));
        glGetProgramInfoLog(handle, logLength, nullptr, (GLchar*)buf.data());
        SPDLOG_ERROR(buf);
    }
    return (GLboolean)status == GL_TRUE;
}

GLuint GL::CreateShader(const GLchar* vertexShader, const GLchar* fragmentShader)
{
    bool ok = true;

    GLuint vertHandle;
    GL_CALL(vertHandle = glCreateShader(GL_VERTEX_SHADER));
    glShaderSource(vertHandle, 1, &vertexShader, nullptr);
    glCompileShader(vertHandle);
    ok &= GL::CheckShader(vertHandle, "vertex shader");

    GLuint fragHandle;
    GL_CALL(fragHandle = glCreateShader(GL_FRAGMENT_SHADER));
    glShaderSource(fragHandle, 1, &fragmentShader, nullptr);
    glCompileShader(fragHandle);
    ok &= GL::CheckShader(fragHandle, "fragment shader");

    // Link
    auto shaderHandle = glCreateProgram();
    glAttachShader(shaderHandle, vertHandle);
    glAttachShader(shaderHandle, fragHandle);
    glLinkProgram(shaderHandle);
    ok &= GL::CheckProgram(shaderHandle, "shader program");

    glDetachShader(shaderHandle, vertHandle);
    glDetachShader(shaderHandle, fragHandle);
    glDeleteShader(vertHandle);
    glDeleteShader(fragHandle);

    if (!ok) {
        glDeleteShader(shaderHandle);
        shaderHandle = 0;
    }

    return shaderHandle;
}
