#include "main_canvas.h"
#include "01_hello_triangle.h"
#include "app.h"

#include <array>
#include <format>
#include <glad/glad.h>
#include <imgui.h>

MainCanvas::MainCanvas() { FillExamplesVector(); }

MainCanvas::~MainCanvas() { }

void MainCanvas::BuildUI()
{
    static constexpr ImGuiWindowFlags windowFlags
        = ImGuiWindowFlags_NoDecoration;
    static constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInner
        | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit;
    
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
    ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
    ImGui::Begin("Main Canvas", nullptr, windowFlags);
    ImGui::BeginTable("Examples##table", 3, tableFlags);
    ImGui::TableSetupColumn(
        "№", ImGuiTableColumnFlags_NoResize);
    ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Open", ImGuiTableColumnFlags_NoResize);
    for (size_t i = 0; i < examples_.size(); ++i) {
        TableRow(i);
    }
    ImGui::EndTable();
    ImGui::End();
}

void MainCanvas::Render()
{
    const auto size = ImGui::GetMainViewport()->Size;
    glViewport(0, 0, size.x, size.y);

    
    auto color = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
    glClearColor(color.x, color.y, color.z, color.w);
    glClear(GL_COLOR_BUFFER_BIT);
}

void MainCanvas::FillExamplesVector()
{
    examples_.emplace_back("Hello triangle",
                           []() -> Canvas* { return new HelloTriangle; });
}

void MainCanvas::TableRow(size_t i)
{
    if (i >= examples_.size()) {
        return;
    }

    auto& example = examples_[i];

    ImGui::PushID(i);
    ImGui::TableNextColumn();
    ImGui::Text("%zu", i);
    ImGui::TableNextColumn();
    ImGui::Text("%s", example.title.c_str());
    ImGui::TableNextColumn();
    if (ImGui::Button(example.canvas ? "Close" : "Open ")) {
        if (example.canvas) {
            App::CloseWindow(example.canvas);
            example.canvas = nullptr;
        } else {
            example.canvas = example.initializer();
            App::OpenWindow(example.canvas);
        }
    }
    ImGui::PopID();
}
