#include <SDL3/SDL.h>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <glad/glad.h>
#include <spdlog/spdlog.h>
#include <string_view>

constexpr int g_count = 1;

SDL_Window* CreateWindow(std::string_view title)
{
    auto* window = SDL_CreateWindow(title.data(), 800, 200,
                                    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
                                        | SDL_WINDOW_TRANSPARENT);
    if (window == nullptr) {
        spdlog::error("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }

    return window;
}

int main()
{
    std::array<SDL_Window*, g_count> windows;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        spdlog::error("Error: SDL_Init(): %s\n", SDL_GetError());
        exit(1);
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    for (size_t i = 0; i < g_count; ++i) {
        windows[i] = CreateWindow("frame_rate_test");
        SDL_SetWindowPosition(windows[i], 100, 100 + 200 * i);
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(windows[0]);
    if (glContext == nullptr) {
        spdlog::error("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        SDL_Quit();
        exit(1);
    }

    SDL_GL_MakeCurrent(windows[0], glContext);
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        spdlog::error("Error: gladLoadGLLoader() failed\n");
        SDL_Quit();
        exit(1);
    }

    SDL_GL_SetSwapInterval(1);

    auto getTicksMs = []() {
        using Clock = std::chrono::high_resolution_clock;
        using Ms = std::chrono::milliseconds;
        return std::chrono::duration_cast<Ms>(Clock::now().time_since_epoch())
            .count();
    };
    constexpr auto frameLength = 16;
    uint64_t       nextFrameStart = getTicksMs() + frameLength;
    bool           frameRendered = false;

    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT
                || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                done = true;
            }
        }

        auto t = getTicksMs();
        if (t > nextFrameStart) {
            nextFrameStart += frameLength;
            frameRendered = false;
        }

        if (!frameRendered) {
            for (size_t i = 0; i < g_count; ++i) {
                auto* window = windows[i];
                SDL_GL_MakeCurrent(window, glContext);
                glClearColor(0.1f, 0.6f, 0.3f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                SDL_GL_SwapWindow(window);
            }
            frameRendered = true;
        }
    }

    SDL_GL_MakeCurrent(nullptr, nullptr);
    SDL_GL_DestroyContext(glContext);
    for (size_t i = 0; i < g_count; ++i) {
        SDL_DestroyWindow(windows[i]);
    }
    SDL_Quit();

    return 0;
}