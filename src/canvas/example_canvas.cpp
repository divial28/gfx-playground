#include "example_canvas.h"

#include <glad/glad.h>
#include <imgui.h>

ExampleCanvas::ExampleCanvas()
{

}

ExampleCanvas::~ExampleCanvas()
{
    
}

void ExampleCanvas::BuildUI()
{
    static int a;
    ImGui::Begin("Hello window");
    ImGui::Text("hello pretty ui");
    ImGui::DragInt("int", &a);
    ImGui::End();
}

void ExampleCanvas::Render()
{
    glClearColor(0.7, 0.2, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
}
