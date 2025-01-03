#include <SDL3/SDL.h>
#include <glad/glad.h>

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

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_Log("Failed to initialize OpenGL context\n");
        return -1;
    }

    // Successfully loaded OpenGL
    SDL_Log("Loaded OpenGL %d.%d\n", GLVersion.major, GLVersion.minor);

    while (1) {
        int finished = 0;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // tried to get rid of black areas on resize, didnt help
            // if(event.window.type == SDL_EVENT_WINDOW_RESIZED) {
            //     int w = 0, h = 0; 
            //     if (SDL_GetWindowSize(window, &w, &h) && w && h) {
            //         glViewport(0, 0, w, h);
            //     }
            //     continue;
            // }
            if (event.type == SDL_EVENT_QUIT) {
                finished = 1;
                break;
            }
        }

        if (finished) {
            break;
        }

        glClearColor(0.7, 0.3, 0.3, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DestroyContext(glcontext);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}
