#pragma once

#include "canvas.h"

class RexTriangleCanvas : public Canvas
{
public:
    RexTriangleCanvas();
    ~RexTriangleCanvas();

    void BuildUI() override;
    void Render() override;

private:
    
};
