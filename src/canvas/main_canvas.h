#pragma once

#include "canvas.h"

class MainCanvas : public Canvas
{
public:
    MainCanvas();
    ~MainCanvas();

    void BuildUI() override;
    void Render() override;
};