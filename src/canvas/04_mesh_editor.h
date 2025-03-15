#pragma once


#include "canvas.h"
#include "utils.h"

#include <array>
#include <glad/glad.h>

class Mode;

class MeshEditorCanvas : public Canvas
{
public:


    enum ModeTag {
        Selection,
        Polygon,
        ModesCount
    };

    MeshEditorCanvas();
    ~MeshEditorCanvas();

    void BuildUI() override;
    void Render() override;

private:

private:
    Color bgColor_ = {};
    ModeTag activeMode_ = Selection;
    std::array<Mode*, ModesCount> modes_;
};
