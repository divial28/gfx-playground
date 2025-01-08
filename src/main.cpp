#include <cstdlib>
#include <glad/glad.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>
#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <string_view>

void InitSDL()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        exit(1);
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
}

SDL_Window* CreatePlatformWindow(std::string_view title)
{
    auto* window = SDL_CreateWindow(title.data(), 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }
    return window;
}

SDL_GLContext CreateGLContext(SDL_Window* window)
{
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (glContext == nullptr)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }
    return glContext;
}

void InitGL(SDL_Window* window, SDL_GLContext glContext)
{
    SDL_GL_MakeCurrent(window, glContext);
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        printf("Error: gladLoadGLLoader() failed\n");
        SDL_Quit();
        exit(1);
    }
    
    SDL_GL_SetSwapInterval(1);
}

ImGuiContext* CreateImGuiContext(SDL_Window* window, SDL_GLContext glContext, ImFontAtlas* fontAtlas, std::string_view iniFilename)
{
    static constexpr const char* glsl_version = "#version 130";
    auto imguiContext = ImGui::CreateContext(fontAtlas);
    ImGui::SetCurrentContext(imguiContext);
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = iniFilename.data();
    io.FontGlobalScale = 2;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsLight();
    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init(glsl_version);
    return imguiContext;
}

void ProcessEvent(SDL_Window* window, ImGuiContext* imguiContext, const SDL_Event& event)
{
    ImGui::SetCurrentContext(imguiContext);
    ImGui_ImplSDL3_ProcessEvent(&event);
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window)) {
        SDL_HideWindow(window);
    }
}

void ImGuiUpdate(ImGuiContext* imguiContext, std::string_view title, float& f, int& counter, bool& checked)
{
    ImGui::SetCurrentContext(imguiContext);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin(title.data());
    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    if (ImGui::Button("Button"))
        counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);
    ImGui::Checkbox("checkbox", &checked);
    auto& io = ImGui::GetIO();
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::End();
    
    ImGui::Render();
}

void Render(SDL_Window* window, SDL_GLContext glContext, ImVec4 clearColor)
{
    SDL_GL_MakeCurrent(window, glContext);
    // glViewport() doesn't affect anything, guess on other platforms might be necessary
    glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w, clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT);
}

void ImGuiRender(SDL_Window* window, SDL_GLContext glContext, ImGuiContext* imguiContext)
{
    ImGui::SetCurrentContext(imguiContext);
    SDL_GL_MakeCurrent(window, glContext);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void SwapBuffers(SDL_Window* window)
{
    SDL_GL_SwapWindow(window);
}

void DestroyImGuiContext(ImGuiContext* imguiContext)
{
    ImGui::SetCurrentContext(imguiContext);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void DestroyPlatformWindow(SDL_Window* window)
{
    SDL_GL_MakeCurrent(window, nullptr);
    SDL_DestroyWindow(window);
}

void DestroyGLContext(SDL_GLContext glContext)
{
    SDL_GL_DestroyContext(glContext);
}    

void ShutdownSDL()
{
    SDL_Quit();
}

int main(int, char**)
{
    InitSDL();
    auto window = CreatePlatformWindow("Viewport 1");
    auto window2 = CreatePlatformWindow("Viewport 2");
    auto glContext = CreateGLContext(window);
    InitGL(window, glContext);

    IMGUI_CHECKVERSION();
    auto fontAtlas = new ImFontAtlas;
    auto imguiContext = CreateImGuiContext(window, glContext, fontAtlas, "imgui.ini");
    auto imguiContext2 = CreateImGuiContext(window2, glContext, fontAtlas, "imgui2.ini");

    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ProcessEvent(window, imguiContext, event); // canvas 1
            ProcessEvent(window2, imguiContext2, event); // canvas 2
            if (event.type == SDL_EVENT_QUIT) {
                done = true;
            }
        }

        { // update and render canvas 1
            static float f = 0.0f;
            static int counter = 0;
            static bool checked;
            ImGuiUpdate(imguiContext, "Window A", f, counter, checked);
            Render(window, glContext, {0.7, 0.4, 0.3, 1.0});
            ImGuiRender(window, glContext, imguiContext);
            SwapBuffers(window);
        }
        { // update and render canvas 2
            static float f = 0.0f;
            static int counter = 0;
            static bool checked;
            ImGuiUpdate(imguiContext2, "Window B", f, counter, checked);
            Render(window2, glContext, {0.2, 0.6, 0.3, 1.0});
            ImGuiRender(window2, glContext, imguiContext2);
            SwapBuffers(window2);
        }
    }

    // Destroy canvas 1
    DestroyImGuiContext(imguiContext);
    DestroyPlatformWindow(window);
    // Destroy canvas 2
    DestroyImGuiContext(imguiContext2);
    DestroyPlatformWindow(window2);
    ShutdownSDL();

    return 0;
}
