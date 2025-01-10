#pragma once

#include "canvas.h"

#include <glad/glad.h>

class HelloTriangle : public Canvas
{
public:
    HelloTriangle();
    ~HelloTriangle();

    void BuildUI() override;
    void Render() override;

private:
    GLuint shader_;
};
