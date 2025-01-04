#pragma once

struct SDL_Window;
struct SDL_GLContextState;

class App
{
public:
    App(int argc = 0, char** argv = nullptr ) { Init(); }
    ~App() { Shutdown(); }
    int Exec();

private:
    void Init();
    bool InitSDL();
    bool InitGL();
    bool InitImGui();
    void ProcessEvents();
    void Update();
    bool UpdateUI();
    void UpdateUIScaling(float scale);
    void Shutdown();

    // debug
    void ShowDisplayInfo();

private:
    SDL_Window* window = nullptr;
    SDL_GLContextState* glcontext = nullptr;
    bool running = true;
    float dpi = 1.25;
};