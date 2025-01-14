#pragma once

#include "canvas.h"
#include "utils.h"

#include <array>
#include <glad/glad.h>

class DrawCommandsCanvas : public Canvas
{
public:
    DrawCommandsCanvas();
    ~DrawCommandsCanvas();

    using mat4 = std::array<float, 16>;

    void BuildUI() override;
    void Render() override;

private:
    GLuint shader_ = 0;
    GLuint modelLoc_ = 0;
    GLuint projectionLoc_ = 0;

    float debugDist_ = -1.0f;
    float near_ = 0.001;
    float far_ = 100;
    bool  projectionCalc_ = true;
    mat4  projection_ = { 0 };

    GLuint vao_ = 0;
    GLuint vboVertices_ = 0;
    GLuint vboColors_ = 0;
    GLuint ebo_ = 0;

    GLuint baseVertex_ = 0;

    float viewportOffsetX_ = 0.0f;
    Color bgColor_ = {};
};
