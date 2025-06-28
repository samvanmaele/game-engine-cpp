#include <glcontext.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_timer.h>
#include <iostream>
#include <platform.hpp>

#ifdef TARGET_PLATFORM_WEB
    #include <emscripten.h>
    #include <emscripten/html5.h>
    #include <GLES3/gl3.h>
#else
    #include <glew/glew.h>
#endif

SDL_Window* InitGLContext(const int screenWidth, const int screenHeight, const int swapInterval)
{
    SDL_Window* window;
    SDL_GLContext mainContext;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "Unable to initialize SDL: " << SDL_GetError() << std::endl;
    }
    if (TTF_Init() == -1)
    {
        std::cerr << "Unable to initialize TTF: " << TTF_GetError() << std::endl;
    }

    std::cout << "Init OpenGL" << std::endl;

    #ifdef TARGET_PLATFORM_WEB
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    #else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    #endif

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window = SDL_CreateWindow("...", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window)
    {
        std::cerr << "Unable to create window: " << SDL_GetError() << std::endl;
    }

    mainContext = SDL_GL_CreateContext(window);
    if (!mainContext)
    {
        std::cerr << "Unable to create OpenGL context: " << SDL_GetError() << std::endl;
    }
    #ifdef TARGET_PLATFORM_WEB
        EmscriptenWebGLContextAttributes attrs = {};
        attrs.antialias = true;
        attrs.majorVersion = 2;
        attrs.minorVersion = 0;
        attrs.alpha = true;
        attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_DEFAULT;

        emscripten_webgl_init_context_attributes(&attrs);
        attrs.majorVersion = 2;
        EMSCRIPTEN_WEBGL_CONTEXT_HANDLE webgl_context = emscripten_webgl_create_context("#canvas", &attrs);
        emscripten_webgl_make_context_current(webgl_context);
    #else
        glewExperimental = GL_TRUE;
        GLenum glewinit = glewInit();
        if(glewinit!=GLEW_OK)
        {
            std::cout << "glewInit failed, aborting" << "\n";
        }
        else
        {
            std::cout << "GLEW initialised" << "\n";
        }
    #endif

    if (SDL_GL_SetSwapInterval(swapInterval) < 0)
    {
        std::cerr << "Unable to set swap interval: " << SDL_GetError() << std::endl;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, screenWidth, screenHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    SDL_GL_SwapWindow(window);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    std::cout << "OpenGL initialized" << std::endl;
    std::cout << "GL_VERSION: " << glGetString(GL_VERSION) << std::endl;

    return window;
}