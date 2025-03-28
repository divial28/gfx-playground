#include "app.h"
#include "canvas/canvas.h"

#include <glad/glad.h>
#include <imgui.h>
#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>
#include <nfd.hpp>

#include "./backends/imgui_impl_opengl3.h"
#include "./backends/imgui_impl_sdl3.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {
struct WindowInfo
{
    SDL_Window*   window;
    ImGuiContext* imguiContext;
    Canvas*       canvas;
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
    void InitLogger();
    void InitPlatform();
    void InitRenderer();
    void InitUI();

    int  Loop();
    bool ProcessEvents();
    void ProcessEvent(WindowInfo& wi, SDL_Event& event);
    void UpdateWindows();

    void Shutdown();
    void ShutdownPlatform();
    void ShutdownRenderer();
    void ShutdownUI();

    bool OpenWindow(Canvas* canvas);
    bool IsOpened(Canvas* canvas) const;
    bool CloseWindow(Canvas* canvas);
    void CloseAllWindows();
    void ProcessWindowsCreateQueue();
    void ProcessWindowsDestroyQueue();

private:
    SDL_Window*   fakeWindow_ = nullptr;
    SDL_GLContext glContext_;
    ImFontAtlas*  fontAtlas_ = nullptr;

    std::unordered_map<Canvas*, WindowInfo> windows_;
    std::vector<WindowInfo> windowsCreateQueue_;
    std::vector<WindowInfo> windowsDestroyQueue_;

    friend class App;
};

AppImpl* g_app = nullptr;
} // namespace

static SDL_Window*
CreatePlatformWindow(std::string_view title, int w = 1280, int h = 720,
                     Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE)
{
    auto* window = SDL_CreateWindow(title.data(), w, h, flags);
    if (window == nullptr) {
        SPDLOG_ERROR("SDL_CreateWindow(): %s\n", SDL_GetError());
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
        SPDLOG_ERROR("SDL_GL_CreateContext(): %s\n", SDL_GetError());
    }
    return glContext;
}

static void DestroyGLContext(SDL_GLContext glContext)
{
    SDL_GL_MakeCurrent(nullptr, nullptr);
    SDL_GL_DestroyContext(glContext);
}

static ImGuiContext* CreateImGuiContext(SDL_Window*      window,
                                        SDL_GLContext    glContext,
                                        ImFontAtlas*     fontAtlas,
                                        std::string_view iniFilename)
{
    auto* lastWindow = SDL_GL_GetCurrentWindow();
    auto* lastImGuiContext = ImGui::GetCurrentContext();

    auto* imguiContext = ImGui::CreateContext(fontAtlas);
    ImGui::SetCurrentContext(imguiContext);
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = iniFilename.data();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsLight();
    ImGuiSDL3InitForOpenGL(window, glContext);
    ImGuiGLInitContext();

    SDL_GL_MakeCurrent(lastWindow, glContext);
    ImGui::SetCurrentContext(lastImGuiContext);

    return imguiContext;
}

static void DestroyImGuiContext(ImGuiContext* imguiContext)
{
    auto* lastWindow = SDL_GL_GetCurrentWindow();
    auto* lastGlContext
        = SDL_GL_GetCurrentContext(); // there is only one context
    auto* lastImGuiContext = ImGui::GetCurrentContext();

    ImGui::SetCurrentContext(imguiContext);
    ImGuiGLShutdownContext();
    ImGuiSDL3Shutdown();
    ImGui::DestroyContext();

    SDL_GL_MakeCurrent(lastWindow, lastGlContext);
    ImGui::SetCurrentContext(lastImGuiContext);
}

AppImpl::AppImpl(int argc, char** argv) { ParseCmdArgs(argc, argv); }

AppImpl::~AppImpl() { }

void AppImpl::ParseCmdArgs(int argc, char** argv) { }

void AppImpl::Init()
{
    InitLogger();
    InitPlatform();
    InitRenderer();
    InitUI();
    NFD_Init();
}

void AppImpl::InitLogger()
{
    // [time with microseconds] [error level] [func:line] msg
    spdlog::set_pattern("[%H:%M:%S.%f] [%^%L%$] [%!:%#] %v");
    spdlog::set_level(spdlog::level::trace);
}

void AppImpl::InitPlatform()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SPDLOG_ERROR("SDL_Init(): %s\n", SDL_GetError());
        exit(1);
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    fakeWindow_ = CreatePlatformWindow("fake window", 0, 0,
                                       SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!fakeWindow_) {
        SPDLOG_ERROR("Could not create any window\n");
        SDL_Quit();
        exit(1);
    }
}

void AppImpl::InitRenderer()
{
    glContext_ = CreateGLContext(fakeWindow_);
    if (!glContext_) {
        SPDLOG_ERROR("Could not create any gl context\n");
        SDL_Quit();
        exit(1);
    }

    SDL_GL_MakeCurrent(fakeWindow_, glContext_);
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SPDLOG_ERROR("gladLoadGLLoader() failed\n");
        SDL_Quit();
        exit(1);
    }

    SDL_GL_SetSwapInterval(1);
}

void AppImpl::InitUI()
{
    fontAtlas_ = new ImFontAtlas();
    fontAtlas_->Clear();
    fontAtlas_->AddFontFromFileTTF("../externals/imgui/misc/fonts/Cousine-Regular.ttf", 24);
    fontAtlas_->Build();
    ImGuiGLInit();
}

int AppImpl::Loop()
{
    bool done = false;
    
    constexpr uint8_t frameLength = 16; // ~60Hz
    uint64_t nextFrameStart = SDL_GetTicks() + frameLength;

    while (!done) {
        ProcessWindowsCreateQueue();
        ProcessWindowsDestroyQueue();
        done = ProcessEvents();

        auto t = SDL_GetTicks();
        if (t > nextFrameStart) {
            nextFrameStart += frameLength;
            UpdateWindows();
        }
    }

    return 0;
}

bool AppImpl::ProcessEvents()
{
    bool      done = false;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        for (auto& window : windows_) {
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
    ImGui::SetCurrentContext(wi.imguiContext);
    ImGuiSDL3ProcessEvent(&event);

    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED
        && event.window.windowID == SDL_GetWindowID(wi.window)) {
        App::CloseWindow(wi.canvas);
    }
}

void AppImpl::UpdateWindows()
{
    for (auto& window : windows_) {
        auto& wi = window.second;

        // build ui
        ImGui::SetCurrentContext(wi.imguiContext);
        ImGuiGLNewFrame();
        ImGuiSDL3NewFrame();
        ImGui::NewFrame();
        wi.canvas->BuildUI();
        ImGui::Render();

        // render content and then ui
        SDL_GL_MakeCurrent(wi.window, glContext_);
        wi.canvas->Render();
        ImGuiGLRenderDrawData(ImGui::GetDrawData());

        // present on screen
        SDL_GL_SwapWindow(wi.window);
    }
}

void AppImpl::Shutdown()
{
    CloseAllWindows();
    ShutdownUI();
    ShutdownRenderer();
    ShutdownPlatform();
    NFD_Quit();
}

void AppImpl::ShutdownPlatform()
{
    DestroyPlatformWindow(fakeWindow_);
    SDL_Quit();
}

void AppImpl::ShutdownRenderer()
{
    SDL_GL_MakeCurrent(nullptr, nullptr);
    SDL_GL_DestroyContext(glContext_);
}

void AppImpl::ShutdownUI() { ImGuiGLShutdown(); }

bool AppImpl::OpenWindow(Canvas* canvas)
{
    assert(glContext_);

    if (!canvas) {
        return false;
    }

    auto it = windows_.find(canvas);
    if (it != windows_.end()) {
        return false;
    }

    WindowInfo wi;
    wi.window = CreatePlatformWindow("");
    if (!wi.window) {
        return false;
    }

    wi.imguiContext = CreateImGuiContext(wi.window, glContext_, fontAtlas_, "");
    if (!wi.imguiContext) {
        return false;
    }

    wi.canvas = canvas;
    windowsCreateQueue_.push_back(wi);
    return true;
}

bool AppImpl::IsOpened(Canvas* canvas) const
{
    return windows_.find(canvas) != windows_.end();
}

bool AppImpl::CloseWindow(Canvas* canvas)
{
    if (!canvas) {
        return false;
    }

    auto it = windows_.find(canvas);
    if (it == windows_.end()) {
        return false;
    }

    windowsDestroyQueue_.push_back(it->second);
    return true;
}

void AppImpl::CloseAllWindows()
{
    for (auto& window : windows_) {
        auto& wi = window.second;
        DestroyImGuiContext(wi.imguiContext);
        DestroyPlatformWindow(wi.window);
        delete wi.canvas;
    }
}

void AppImpl::ProcessWindowsCreateQueue()
{
    for (auto& wi: windowsCreateQueue_) {
        windows_[wi.canvas] = wi;
    }
    windowsCreateQueue_.clear();
}

void AppImpl::ProcessWindowsDestroyQueue()
{
    for (auto& wi: windowsDestroyQueue_) {
        auto it = windows_.find(wi.canvas); // 100% there
        windows_.erase(it);
        DestroyImGuiContext(wi.imguiContext);
        DestroyPlatformWindow(wi.window);
        delete wi.canvas; // FIXME
    }
    windowsDestroyQueue_.clear();
}

App::App(int argc, char** argv)
{
    g_app = new AppImpl(argc, argv);
    g_app->Init();
}

App::~App() { delete g_app; }

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

bool App::IsOpened(Canvas* canvas)
{
    assert(g_app);
    return g_app->IsOpened(canvas);
}

bool App::CloseWindow(Canvas* canvas)
{
    assert(g_app);
    return g_app->CloseWindow(canvas);
}
