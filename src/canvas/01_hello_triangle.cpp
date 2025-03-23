#include "01_hello_triangle.h"
#include "gl/framework.h"
#include "utils.h"

#include <imgui.h>


HelloTriangleCanvas::HelloTriangleCanvas() 
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

HelloTriangleCanvas::~HelloTriangleCanvas() 
{
    glDeleteProgram(shader_);
}

void HelloTriangleCanvas::BuildUI()
{
    static int a;
    ImGui::Begin("Hello window");
    ImGui::Text("hello pretty ui");
    ImGui::DragInt("int", &a);
    ImGui::End();
}

void HelloTriangleCanvas::Render()
{
    const auto size = ImGui::GetMainViewport()->Size;
    glViewport(0, 0, size.x, size.y);
    glClearColor(bgColor_.r, bgColor_.g, bgColor_.b, bgColor_.a);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
