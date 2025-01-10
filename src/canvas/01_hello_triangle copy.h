#pragma once

#include "canvas.h"

#include <glad/glad.h>

class HelloTriangleCanvas : public Canvas
{
public:
    HelloTriangleCanvas();
    ~HelloTriangleCanvas();

    void BuildUI() override;
    void Render() override;

private:
    GLuint shader_;
};
