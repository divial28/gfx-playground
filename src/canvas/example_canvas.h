#pragma once

#include "canvas.h"

class ExampleCanvas : public Canvas
{
public:
    ExampleCanvas();
    ~ExampleCanvas();

    void BuildUI() override;
    void Render() override;
};