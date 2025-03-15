#pragma once

#include <imgui.h>

class Mode
{
public:
    virtual void OnMouseClick(ImVec2 pos, bool button[ImGuiMouseButton_COUNT]) = 0;
};

class SelectionMode : public Mode
{
public:
    void OnMouseClick(ImVec2 pos, bool button[ImGuiMouseButton_COUNT]) override;
};

class PolygonMode : public Mode
{
public:
    void OnMouseClick(ImVec2 pos, bool button[ImGuiMouseButton_COUNT]) override;
};
