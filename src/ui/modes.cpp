#include "modes.h"
#include <spdlog/spdlog.h>

void SelectionMode::OnMouseClick(ImVec2 pos, bool button[ImGuiMouseButton_COUNT])
{
    if (button [ImGuiMouseButton_Left]) {
        SPDLOG_TRACE("selection click");
    }
}
        

void PolygonMode::OnMouseClick(ImVec2 pos, bool button[ImGuiMouseButton_COUNT])
{
    if (button [ImGuiMouseButton_Left]) {
        SPDLOG_TRACE("polygon click");
    }
}