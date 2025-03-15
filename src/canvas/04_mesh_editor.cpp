#include "04_mesh_editor.h"

#include "ui/modes.h"
#include "gl/framework.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <spdlog/spdlog.h>



MeshEditorCanvas::MeshEditorCanvas() 
{
    bgColor_ = Color::Convert(0xB0BEC5ff);
    modes_ = {
        new SelectionMode,
        new PolygonMode
    };

    /* shader_ = GL::CreateShader(
        R"(#version 460 core
        layout (location = 0) in vec2 vPos;
        layout (location = 1) in vec3 vColor;
        out vec3 fColor;
        void main()
        {
            fColor = aColor;
            gl_Position = vec4(vPos, 0.0, 1.0f);
        })",
        R"(#version 460 core
        in vec3 fColor;
        out vec4 color;
        void main()
        {
            color = vec4(fColor, 1.0f);
        })"
    ); */
}

MeshEditorCanvas::~MeshEditorCanvas() 
{
}

void MeshEditorCanvas::BuildUI()
{
    ImGui::BeginMainMenuBar();
    ImGui::RadioButton("Selection", (int*)&activeMode_, Selection);
    ImGui::RadioButton("Polygon", (int*)&activeMode_, Polygon);
    ImGui::EndMainMenuBar();

    auto& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    
    auto m = ImGui::GetMousePos();
    modes_[activeMode_]->OnMouseClick(m, io.MouseClicked);
}

void MeshEditorCanvas::Render()
{
    const auto size = ImGui::GetMainViewport()->Size;
    glViewport(0, 0, size.x, size.y);
    glClearColor(bgColor_.r, bgColor_.g, bgColor_.b, bgColor_.a);
    glClear(GL_COLOR_BUFFER_BIT);

}
