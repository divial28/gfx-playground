/**
 * Modify this source such that it reproduces your problem.
 */

/* START of source modifications */

#include <SDL3/SDL.h>
/*
 * SDL3/SDL_main.h is explicitly not included such that a terminal window would appear on Windows.
 */

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed (%s)", SDL_GetError());
        return 1;
    }

    SDL_Window *window = nullptr;

    if (!(window = SDL_CreateWindow("Graphics playground", 640, 480, 0))) {
        SDL_Log("SDL_CreateWindow (%s)", SDL_GetError());
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

    }

    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}

/* END of source modifications */