#include "02_draw_commands.h"
#include "gl/framework.h"

#include <imgui.h>


DrawCommandsCanvas::DrawCommandsCanvas() 
{
    shader_ = GL::CreateShader(
        R"(#version 460 core
        in vec3 pos;
        void main()
        {
            gl_Position = vec4(pos, 1.0);
        })",
        R"(#version 460 core
        out vec4 color;
        void main()
        {
            color = vec4(1.0, 0.0, 0.0, 1.0);
        })"
    );

    glCreateBuffers(1, &vao_);
    glCreateBuffers(1, &vbo_);
    glCreateBuffers(1, &ebo_);

}

DrawCommandsCanvas::~DrawCommandsCanvas() 
{
    glDeleteBuffers(1, &ebo_);
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &vao_);
}

void DrawCommandsCanvas::BuildUI()
{
    ImGui::Begin("settings");
    ImGui::End();
}

void DrawCommandsCanvas::Render()
{
    const auto size = ImGui::GetMainViewport()->Size;
    glViewport(0, 0, size.x, size.y);
    glClearColor(bgColor_.r, bgColor_.g, bgColor_.b, bgColor_.a);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
