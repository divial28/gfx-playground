#include "main_canvas.h"
#include "app.h"
#include "example_canvas.h"

#include <glad/glad.h>
#include <imgui.h>

MainCanvas::MainCanvas() { }

MainCanvas::~MainCanvas() { }

void MainCanvas::BuildUI()
{
    ImGui::ShowDemoWindow();
    ImGui::Begin("qwe");

    static ExampleCanvas* canvas = nullptr;
    if (ImGui::Button("Example canvas")) {
        if (!canvas) {
            canvas = new ExampleCanvas;
            App::OpenWindow(canvas);
        } else {
            App::CloseWindow(canvas);
            canvas = nullptr;
        }
    }
    ImGui::End();
}

void MainCanvas::Render()
{
    glClearColor(0.3, 0.6, 0.3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
}