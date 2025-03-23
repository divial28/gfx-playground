#include "app.h"
#include "canvas/01_hello_triangle.h"
#include "canvas/02_draw_commands.h"
#include "canvas/03_dsa_buffers.h"
#include "canvas/04_mesh_editor.h"
#include "canvas/05_texture_compression.h"
#include "main_canvas.h"

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
                           []() -> Canvas* { return new HelloTriangleCanvas; });
    examples_.emplace_back("Draw Commands",
                           []() -> Canvas* { return new DrawCommandsCanvas; });
    examples_.emplace_back("DSA buffers",
                           []() -> Canvas* { return new DsaBuffersCanvas; });
    examples_.emplace_back("Mesh editor",
                           []() -> Canvas* { return new MeshEditorCanvas; });
    examples_.emplace_back("Texture compression",
                           []() -> Canvas* { return new TextureCompressionCanvas; });
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

    bool shown = App::IsOpened(example.canvas);
    example.canvas = shown ? example.canvas : nullptr;
    
    if (ImGui::Button(shown ? "Close" : "Open ")) {
        if (shown) {
            App::CloseWindow(example.canvas);
            example.canvas = nullptr;
        } else {
            example.canvas = example.initializer();
            App::OpenWindow(example.canvas);
        }
    }
    ImGui::PopID();
}
