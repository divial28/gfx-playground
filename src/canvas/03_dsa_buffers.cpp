#include "03_dsa_buffers.h"
#include "gl/framework.h"

#include <imgui.h>
#include <spdlog/spdlog.h>

DsaBuffersCanvas::DsaBuffersCanvas() 
{
    bgColor_ = Color::Convert(0xB0BEC5ff);
    shader_ = GL::CreateShader(
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
    );
}

DsaBuffersCanvas::~DsaBuffersCanvas() 
{
    DestroyBuffers();
    glDeleteProgram(shader_);
}

void DsaBuffersCanvas::BuildUI()
{ 
    ImGui::Begin("settings");
    if (ImGui::Button("Create buffers")) {
        CreateBuffers();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Use DSA", &useDsa_);
    if (ImGui::Button("Destroy buffers")) {
        DestroyBuffers();
    }
    ImGui::End();
}

void DsaBuffersCanvas::Render()
{
    const auto size = ImGui::GetMainViewport()->Size;
    glViewport(0, 0, size.x, size.y);
    glClearColor(bgColor_.r, bgColor_.g, bgColor_.b, bgColor_.a);
    glClear(GL_COLOR_BUFFER_BIT);

    if (vao_) {
        GL_CALL(glBindVertexArray(vao_));
        GL_CALL(glUseProgram(shader_));  
        GL_CALL(glDrawArrays(GL_TRIANGLES, 0, 3));
        // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
    }
}

void DsaBuffersCanvas::CreateBuffers()
{
    static constexpr GLfloat vertices[] =
    {
        -0.4f, -0.4f, 1.0f, 0.0f, 0.0f,
         0.4f, -0.4f, 0.0f, 1.0f, 0.0f,
        -0.4f,  0.4f, 0.0f, 0.0f, 1.0f,
         0.4f,  0.4f, 1.0f, 1.0f, 0.0f,
    };

    static const GLubyte indices[] = { 0, 1, 2, 1, 2, 3 };

    DestroyBuffers();
    if (useDsa_) {
        CreateBuffersDsa(vertices, indices);
    } else {
        CreateBuffersNonDsa(vertices, indices);
    }
}

void DsaBuffersCanvas::CreateBuffersDsa(const GLfloat* vertices, const GLubyte* indices)
{
    glCreateBuffers(1, &vbo_);
    glNamedBufferStorage(vbo_, sizeof(GLfloat) * 20, vertices, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &ebo_);
    glNamedBufferStorage(ebo_, sizeof(GLubyte) * 6, indices, GL_DYNAMIC_STORAGE_BIT);
    
    glCreateVertexArrays(1, &vao_);
    glVertexArrayVertexBuffer(vao_, 0, vbo_, 0, sizeof(float) * 5);
    glVertexArrayElementBuffer(vao_, ebo_);

    glEnableVertexArrayAttrib(vao_, 0);
    glEnableVertexArrayAttrib(vao_, 1);
    glVertexArrayAttribFormat(vao_, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(vao_, 1, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat));
    glVertexArrayAttribBinding(vao_, 0, 0);
    glVertexArrayAttribBinding(vao_, 1, 0);
}

void DsaBuffersCanvas::CreateBuffersNonDsa(const GLfloat* vertices, const GLubyte* indices)
{
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 20, vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &ebo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 6, indices, GL_STATIC_DRAW);
    
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void DsaBuffersCanvas::DestroyBuffers()
{
    if (vao_) {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0;
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
        glDeleteBuffers(1, &vao_);
        vao_ = 0;
    }
}
