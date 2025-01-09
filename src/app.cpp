#include "app.h"
#include "canvas/canvas.h"

#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <imgui.h>

#include "./backends/imgui_impl_opengl3.h"
#include "./backends/imgui_impl_sdl3.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <unordered_map>

namespace {
    struct WindowInfo
    {
        SDL_Window* window_;
        ImGuiContext* imguiContext_;
        Canvas* canvas_;
    };

    class AppImpl
    {
    public:
        AppImpl(int argc, char** argv);
        ~AppImpl();
        AppImpl(const AppImpl&) = delete;
        AppImpl(AppImpl&&) = delete;
        AppImpl& operator=(const AppImpl&) = delete;
        AppImpl& operator=(AppImpl&&) = delete;

    public:
        void ParseCmdArgs(int argc, char** argv);
        void Init();
        void InitPlatform();
        void InitRenderer();
        void InitUI();

        int Loop();
        bool ProcessEvents();
        void ProcessEvent(WindowInfo& wi, SDL_Event& event);
        void UpdateWindows();

        void Shutdown();
        void ShutdownPlatform();
        void ShutdownRenderer();

        bool OpenWindow(Canvas* canvas);
        // bool ShowWindow(Canvas* canvas);
        // bool HideWindow(Canvas* canvas);
        bool CloseWindow(Canvas* canvas);
        void CloseAllWindows();

    private:
        SDL_Window* fakeWindow = nullptr;
        SDL_GLContext glContext;
        ImFontAtlas* fontAtlas = nullptr;

        std::unordered_map<Canvas*, WindowInfo> windows;

        friend class App;
    };

    AppImpl* g_app = nullptr;
}

static SDL_Window* CreatePlatformWindow(std::string_view title, int w = 1280, int h = 720, Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE)
{
    auto* window = SDL_CreateWindow(title.data(), w, h, flags);
    if (window == nullptr) {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
    }
    return window;
}

static void DestroyPlatformWindow(SDL_Window* window)
{
    if (SDL_GL_GetCurrentWindow() == window) {
        SDL_GL_MakeCurrent(nullptr, nullptr);
    }
    SDL_DestroyWindow(window);
}

static SDL_GLContext CreateGLContext(SDL_Window* window)
{
    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (glContext == nullptr) {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
    }
    return glContext;
}

static void DestroyGLContext(SDL_GLContext glContext)
{
    SDL_GL_MakeCurrent(nullptr, nullptr);
    SDL_GL_DestroyContext(glContext);
}

static ImGuiContext* CreateImGuiContext(SDL_Window* window, SDL_GLContext glContext, ImFontAtlas* fontAtlas, std::string_view iniFilename)
{

    auto lastWindow = SDL_GL_GetCurrentWindow();
    auto lastImGuiContext = ImGui::GetCurrentContext();

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
    ImGui_ImplOpenGL3_Init();

    SDL_GL_MakeCurrent(lastWindow, glContext);
    ImGui::SetCurrentContext(lastImGuiContext);

    return imguiContext;
}

static void DestroyImGuiContext(ImGuiContext* imguiContext)
{
    auto lastWindow = SDL_GL_GetCurrentWindow();
    auto lastGlContext = SDL_GL_GetCurrentContext(); // there is only one context
    auto lastImGuiContext = ImGui::GetCurrentContext();

    ImGui::SetCurrentContext(imguiContext);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_MakeCurrent(lastWindow, lastGlContext);
    ImGui::SetCurrentContext(lastImGuiContext);
}

AppImpl::AppImpl(int argc, char** argv)
{
    ParseCmdArgs(argc, argv);
}

AppImpl::~AppImpl()
{
    
}

void AppImpl::ParseCmdArgs(int argc, char** argv)
{

}

void AppImpl::Init()
{
    InitPlatform();
    InitRenderer();
    InitUI();
}

void AppImpl::InitPlatform()
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

    fakeWindow = CreatePlatformWindow("fake window", 0, 0, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!fakeWindow) {
        printf("Could not create any window\n");
        SDL_Quit();
        exit(1);
    }
}

void AppImpl::InitRenderer()
{
    glContext = CreateGLContext(fakeWindow);
    if (!glContext) {
        printf("Could not create any gl context\n");
        SDL_Quit();
        exit(1);
    }

    SDL_GL_MakeCurrent(fakeWindow, glContext);
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        printf("Error: gladLoadGLLoader() failed\n");
        SDL_Quit();
        exit(1);
    }
    
    SDL_GL_SetSwapInterval(1);
}

void AppImpl::InitUI()
{
    fontAtlas = new ImFontAtlas();
}

int AppImpl::Loop()
{
    bool done = false;
    while (!done)
    {
        done = ProcessEvents();
        UpdateWindows();
    }

    return 0;
}

bool AppImpl::ProcessEvents()
{
    bool done = false;
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        for (auto& window : windows) {
            ProcessEvent(window.second, event);
        }
        if (event.type == SDL_EVENT_QUIT) {
            done = true;
        }
    }
    return done;
}

void AppImpl::ProcessEvent(WindowInfo& wi, SDL_Event& event)
{
    ImGui::SetCurrentContext(wi.imguiContext_);
    ImGui_ImplSDL3_ProcessEvent(&event);

    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
        event.window.windowID == SDL_GetWindowID(wi.window_)) {
        SDL_HideWindow(wi.window_);
    }
}

void AppImpl::UpdateWindows()
{
    for (auto& window : windows) {
        auto& wi = window.second;
        
        // build ui
        ImGui::SetCurrentContext(wi.imguiContext_);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        wi.canvas_->BuildUI();
        ImGui::Render();

        // render content and then ui
        SDL_GL_MakeCurrent(wi.window_, glContext);
        wi.canvas_->Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // present on screen
        SDL_GL_SwapWindow(wi.window_);
    }
}

void AppImpl::Shutdown()
{
    CloseAllWindows();
    ShutdownRenderer();
    ShutdownPlatform();
}

void AppImpl::ShutdownPlatform()
{
    DestroyPlatformWindow(fakeWindow);
    SDL_Quit();
}

void AppImpl::ShutdownRenderer()
{
    SDL_GL_MakeCurrent(nullptr, nullptr);
    SDL_GL_DestroyContext(glContext);
}

bool AppImpl::OpenWindow(Canvas* canvas)
{
    assert(glContext);

    if (!canvas) {
        return false;
    }

    auto it = windows.find(canvas);
    if (it != windows.end()) {
        return false;
    }

    WindowInfo wi;
    wi.window_ = CreatePlatformWindow("");
    if (!wi.window_) {
        return false;
    }
    
    wi.imguiContext_ = CreateImGuiContext(wi.window_, glContext, fontAtlas, "");
    if (!wi.imguiContext_) {
        return false;
    }

    wi.canvas_ = canvas;
    windows[canvas] = wi;
    return true;
}

bool AppImpl::CloseWindow(Canvas* canvas)
{
    if (!canvas) {
        return false;
    }

    auto it = windows.find(canvas);
    if (it == windows.end()) {
        return false;
    }

    auto& wi = it->second;
    DestroyImGuiContext(wi.imguiContext_);
    DestroyPlatformWindow(wi.window_);
    windows.erase(it);
    delete canvas; // FIXME

    return true;
}

void AppImpl::CloseAllWindows()
{
    for (auto& window : windows) {
        auto& wi = window.second;
        DestroyImGuiContext(wi.imguiContext_);
        DestroyPlatformWindow(wi.window_);
        delete wi.canvas_;
    }
}

App::App(int argc, char** argv)
{
    g_app = new AppImpl(argc, argv);
    g_app->Init();
}

App::~App()
{
    delete g_app;
}

int App::Exec()
{
    assert(g_app);

    auto res = 0;
    res = g_app->Loop();
    g_app->Shutdown();
    return res;
}

bool App::OpenWindow(Canvas* canvas)
{
    assert(g_app);
    return g_app->OpenWindow(canvas);
}

bool App::CloseWindow(Canvas* canvas)
{
    assert(g_app);
    return g_app->CloseWindow(canvas);
}

