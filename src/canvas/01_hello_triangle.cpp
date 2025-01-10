#include "01_hello_triangle.h"
#include "gl/framework.h"

#include <imgui.h>


HelloTriangle::HelloTriangle() 
{
    shader_ = GL::CreateShader(
        R"(#version 460 core
        void main()
        {
            vec2 points[3] = vec2[3](
                vec2(-0.7, -0.7),
                vec2(0.0, 0.7),
                vec2(0.7, -0.7)
            );
            gl_Position = vec4(points[gl_VertexID], 0.0, 1.0);
        })",
        R"(#version 460 core
        out vec4 color;
        void main()
        {
            color = vec4(1.0, 0.0, 0.0, 1.0);
        })"
    );
}

HelloTriangle::~HelloTriangle() { }

void HelloTriangle::BuildUI()
{
    static int a;
    ImGui::Begin("Hello window");
    ImGui::Text("hello pretty ui");
    ImGui::DragInt("int", &a);
    ImGui::End();
}

void HelloTriangle::Render()
{
    const auto size = ImGui::GetMainViewport()->Size;
    glViewport(0, 0, size.x, size.y);
    glClearColor(0.7, 0.2, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
