#include "SDL3/SDL_video.h"
#include <SDL3/SDL.h>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed (%s)", SDL_GetError());
        return 1;
    }

    auto window = SDL_CreateWindow("Graphics playground", 640, 480,
                                   SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) {
        SDL_Log("SDL_CreateWindow (%s)", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    auto glcontext = SDL_GL_CreateContext(window);
    if (!glcontext) {
        SDL_Log("SDL_GL_CreateContext (%s)", SDL_GetError());
        SDL_Quit();
        return 1;
    }



    while (1) {
        int finished = 0;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                finished = 1;
                break;
            }
        }
        if (finished) {
            break;
        }

        // glClearColor(0,0,0,1);
        // glClear(GL_COLOR_BUFFER_BIT);
        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DestroyContext(glcontext);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}
