#pragma once

#include "canvas.h"
#include "utils.h"

#include <glad/glad.h>

class DrawCommandsCanvas : public Canvas
{
public:
    DrawCommandsCanvas();
    ~DrawCommandsCanvas();

    void BuildUI() override;
    void Render() override;

private:
    GLuint shader_;
    GLuint vao_;
    GLuint vbo_;
    GLuint ebo_;

    Color bgColor_ = Utils::GetNextColorFromPalette();
};
