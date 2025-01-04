#include "app.h"

#include <stdlib.h>

#include <SDL3/SDL.h>
#include <glad/glad.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>

#define SDL_HINT_VIDEO_DISPLAY_PRIORITY "SDL_VIDEO_DISPLAY_PRIORITY"

int App::Exec()
{
    while (running) {
        ProcessEvents();
        Update();
    }
    return 0;
}

#define EXIT_ON_FAIL(expression) \
    if (!(expression)) \
        exit(0)

void App::Init()
{
    EXIT_ON_FAIL(InitSDL());
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED); 
    SDL_ShowWindow(window);
    EXIT_ON_FAIL(InitGL());
    EXIT_ON_FAIL(InitImGui());
}

bool App::InitSDL()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed (%s)", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;

    window = SDL_CreateWindow("Graphics playground", 1280, 720, window_flags);
    if (!window) {
        SDL_Log("SDL_CreateWindow (%s)", SDL_GetError());
        SDL_Quit();
        return false;
    }

    return true;
}

bool App::InitGL()
{
    glcontext = SDL_GL_CreateContext(window);
    if (!glcontext) {
        SDL_Log("SDL_GL_CreateContext (%s)", SDL_GetError());
        SDL_Quit();
        return false;
    }

    SDL_GL_MakeCurrent(window, glcontext);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_Log("Failed to initialize OpenGL context\n");
        return false;
    }

    // Successfully loaded OpenGL
    SDL_Log("Loaded OpenGL %d.%d\n", GLVersion.major, GLVersion.minor);
    return true;
}

bool App::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    const char* glsl_version = "#version 460";
    ImGui_ImplSDL3_InitForOpenGL(window, glcontext);
    ImGui_ImplOpenGL3_Init(glsl_version);

    UpdateUIScaling(1.25);

    return true;
}

void App::ProcessEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT)
            // SDL_Log("SDL_EVENT_QUIT");
            running = false;
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
            // SDL_Log("SDL_EVENT_WINDOW_CLOSE_REQUESTED");
            running = false;
        if (event.type == SDL_EVENT_WINDOW_DISPLAY_CHANGED)
            ShowDisplayInfo();
    }

    // if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
    // {
    //     SDL_Delay(10);
    //     continue;
    // }
}

void App::Update()
{
    // Start the Dear ImGui frame
    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    bool needsRescale = UpdateUI();

    ImGui::Render();
    auto& io = ImGui::GetIO();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }

    SDL_GL_SwapWindow(window);

    if (needsRescale) {
        UpdateUIScaling(dpi);
    }
}

bool App::UpdateUI()
{
    auto res = false;

    ImGui::ShowDemoWindow();
    ImGui::SetNextWindowSize({300, 80}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos, ImGuiCond_FirstUseEver);
    ImGui::Begin("DPI scale", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::SliderFloat("##dpi", &dpi, 0.5, 3.0);
    ImGui::SameLine();
    if (ImGui::Button("Apply")) {
        res = true;
    }
    ImGui::End();

    return res;
}

void App::UpdateUIScaling(float scale)
{
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplOpenGL3_DestroyDeviceObjects();
    
    // Setup Dear ImGui style
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiStyle styleold = style; // Backup colors
    style = ImGuiStyle(); // IMPORTANT: ScaleAllSizes will change the original size, so we should reset all style config
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize  = 1.0f;
    style.PopupBorderSize  = 1.0f;
    style.FrameBorderSize  = 1.0f;
    style.TabBorderSize    = 1.0f;
    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 0.0f;
    style.PopupRounding     = 0.0f;
    style.FrameRounding     = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding      = 0.0f;
    style.TabRounding       = 0.0f;
    style.ScaleAllSizes(scale);
    memcpy(style.Colors, styleold.Colors, sizeof(style.Colors)); // Restore colors
    
    io.Fonts->Clear();
    
    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
    ImFont* font = io.Fonts->AddFontFromFileTTF("../externals/imgui/misc/fonts/Cousine-Regular.ttf", 16.0f * scale);
    IM_ASSERT(font != NULL);

    ImGui_ImplOpenGL3_CreateDeviceObjects();
}

void App::Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(glcontext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void App::ShowDisplayInfo()
{
    auto display = SDL_GetDisplayForWindow(window);
    int w = 0, h = 0;
    SDL_Log("display: %s", SDL_GetDisplayName(display));
    SDL_GetWindowSize(window, &w, &h);
    SDL_Log("SDL_GetWindowSize: %d %d", w, h);
    SDL_GetWindowSizeInPixels(window, &w, &h);
    SDL_Log("SDL_GetWindowSizeInPixels: %d %d", w, h);
    SDL_Log("SDL_GetDisplayContentScale: %f", SDL_GetDisplayContentScale(display));
    SDL_Log("SDL_GetWindowDisplayScale: %f", SDL_GetWindowDisplayScale(window));
    SDL_Log("SDL_GetWindowPixelDensity: %f", SDL_GetWindowPixelDensity(window));
}
