#pragma once

#pragma once

#include "canvas.h"
#include "utils.h"

#include <array>
#include <glad/glad.h>

struct VertexInfo
{
    GLfloat* data;
    // GLuint verte
};

class DsaBuffersCanvas : public Canvas
{
public:
    DsaBuffersCanvas();
    ~DsaBuffersCanvas();

    using mat4 = std::array<float, 16>;

    void BuildUI() override;
    void Render() override;

private:
    void CreateBuffers();
    void CreateBuffersDsa(const GLfloat* vertices, const GLubyte* indices);
    void CreateBuffersNonDsa(const GLfloat* vertices, const GLubyte* indices);
    void DestroyBuffers();

private:
    GLuint shader_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ebo_ = 0;

    bool useDsa_ = true;

    Color bgColor_ = {};
};
