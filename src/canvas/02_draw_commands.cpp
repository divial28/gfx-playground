#include "02_draw_commands.h"
#include "gl/framework.h"

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

DrawCommandsCanvas::DrawCommandsCanvas() 
{
    bgColor_ = Color::Convert(0xB0BEC5ff);
    shader_ = GL::CreateShader(
        R"(#version 460 core
        layout (location = 0) in vec4 vPos;
        layout (location = 1) in vec4 vColor;
        out vec4 fColor;
        uniform mat4 model;
        uniform mat4 projection;
        void main()
        {
            fColor = vColor;
            gl_Position = projection * (model * vPos);
        })",
        R"(#version 460 core
        in vec4 fColor;
        out vec4 color;
        void main()
        {
            color = fColor;
        })"
    );
    modelLoc_ = glGetUniformLocation(shader_, "model");
    projectionLoc_ = glGetUniformLocation(shader_, "projection");

    // A single triangle
    static const GLfloat vertexPositions[] =
    {
        -0.4f, -0.4f,  0.0f, 1.0f,
         0.4f, -0.4f,  0.0f, 1.0f,
        -0.4f,  0.4f,  0.0f, 1.0f,
         0.4f,  0.4f,  0.0f, 1.0f,
    };

    // Color for each vertex
    static const GLfloat vertexColors[] =
    {
        1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f,
    };

    static const GLuint indices[] = { 0, 1, 2, 1, 2, 3 };

    glGenBuffers(1, &vboVertices_);
    glBindBuffer(GL_ARRAY_BUFFER, vboVertices_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);
    glGenBuffers(1, &vboColors_);
    glBindBuffer(GL_ARRAY_BUFFER, vboColors_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors, GL_STATIC_DRAW);
    glGenBuffers(1, &ebo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBindBuffer(GL_ARRAY_BUFFER, vboVertices_);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vboColors_);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr); // tightly packed, same as 4 * sizeof(float)
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

DrawCommandsCanvas::~DrawCommandsCanvas() 
{
    glDeleteBuffers(1, &ebo_);
    glDeleteBuffers(1, &vboVertices_);
    glDeleteBuffers(1, &vboColors_);
    glDeleteBuffers(1, &vao_);
    glDeleteProgram(shader_);
}

using mat4 = DrawCommandsCanvas::mat4;
static mat4 Frustum(float l, float r, float b, float t, float n, float f)
{
    return {
        2.0f*n/(r-l), 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f*n/(t-b), 0.0f, 0.0f,
        (r+l)/(r-l), (t+b)/(t-b), -(f+n)/(f-n), -1.0f,
        0.0f, -(2.0f*f*n)/(f-n), 0.0f, 1.0f
    };
}

static inline mat4 Ortho(float l, float r, float b, float t, float n, float f)
{
    return {2.0f/(r-l), 0.0f,       0.0f,        -(r+l)/(r-l),
            0.0f,       2.0f/(t-b), 0.0f,        -(t+b)/(t-b),
            0.0f,       0.0f,       -2.0f/(f-n), (f+n)/(f-n), // can invert z here
            0.0f,       0.0f,       0.0f,        1.0f};
}

void DrawCommandsCanvas::BuildUI()
{
    ImGui::Begin("settings");
    ImGui::SliderFloat("left offset", &viewportOffsetX_, 0,
                       ImGui::GetWindowViewport()->Size.x / 2);
    ImGui::SliderInt("base vertex", (int*)&baseVertex_, 0, 1);
    ImGui::SliderFloat("dist debug", &debugDist_, -10.0f, 10.0f, "%.3f");
    ImGui::SliderFloat("near", &near_, 0.001f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
    ImGui::SliderFloat("far", &far_, 1.0f, 1000.0f, "%.1f", ImGuiSliderFlags_Logarithmic);
    ImGui::Checkbox("calculate projection automatically", &projectionCalc_);

    if (projectionCalc_) {
        const auto& size = ImGui::GetMainViewport()->Size;
        float aspect = (size.x-viewportOffsetX_)/(size.y);
        // projection_ = Ortho(-aspect, aspect, -1.0f, 1.0f, near_, far_);
        auto mat = glm::ortho(-aspect, aspect, -1.0f, 1.0f, near_, far_);
        memcpy(&projection_[0], &mat[0], 16 * sizeof(float));
    }

    ImGui::BeginDisabled(projectionCalc_);
    ImGui::SliderFloat4("projection## 0", &projection_[0], -5.0f, 5.0f, "%.5f");
    ImGui::SliderFloat4("##projection 1", &projection_[4], -5.0f, 5.0f, "%.5f");
    ImGui::SliderFloat4("##projection 2", &projection_[8], -5.0f, 5.0f, "%.5f");
    ImGui::SliderFloat4("##projection 3", &projection_[12], -5.0f, 5.0f, "%.5f");
    ImGui::EndDisabled();
    ImGui::End();

    const auto maxOffset = ImGui::GetWindowViewport()->Size.x / 2;
    viewportOffsetX_ = viewportOffsetX_ > maxOffset ? maxOffset : viewportOffsetX_;
}

void DrawCommandsCanvas::Render()
{
    const auto size = ImGui::GetMainViewport()->Size;
    glViewport(viewportOffsetX_, 0, size.x - viewportOffsetX_, size.y);
    glClearColor(bgColor_.r, bgColor_.g, bgColor_.b, bgColor_.a);
    glClear(GL_COLOR_BUFFER_BIT);

    float model[] = {
        1.0, 0.0, 0.0, -0.5,
        0.0, 1.0, 0.0, 0.5,
        0.0, 0.0, 1.0, debugDist_,
        0.0, 0.0, 0.0, 1.0,
    };

    glBindVertexArray(vao_);
    glUseProgram(shader_);
    glUniformMatrix4fv(projectionLoc_, 1, GL_TRUE, projection_.data()); // gl uses col major so we need to transpose it

    glUniformMatrix4fv(modelLoc_, 1, GL_TRUE, model);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    model[3] = 0.5;
    glUniformMatrix4fv(modelLoc_, 1, GL_TRUE, model);
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
    
    model[3] = -0.5;
    model[7] = -0.5;
    glUniformMatrix4fv(modelLoc_, 1, GL_TRUE, model);
    glDrawElementsBaseVertex(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0, baseVertex_);

    model[3] = 0.5;
    glUniformMatrix4fv(modelLoc_, 1, GL_TRUE, model); // gl uses col major so we need to transpose it
    glDrawArraysInstanced(GL_TRIANGLES, 0, 3, 1);
}
