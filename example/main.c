#include <sdl_media_sink.h>

static SDL_bool handle_event(const SDL_Event *event);

int main()
{
    SDL_Window *window;
    SDL_GLContext *gl_context;
    SDL_MediaSink *media_sink;
    SDL_bool active;
    SDL_Event event;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    window = SDL_CreateWindow(
        "SDL Media Sink",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        640,
        480,
        SDL_WINDOW_OPENGL);
    SDL_assert(window != NULL);

    gl_context = SDL_GL_CreateContext(window);
    SDL_assert(gl_context != NULL);

    media_sink = SDL_CreateMediaSink(window, gl_context, NULL);
    SDL_assert(media_sink != NULL);

    SDL_assert(SDL_AttachMediaSink(media_sink, SDL_MEDIA_TYPE_TEST, NULL, 30) == 0);

    SDL_assert(SDL_PlayMediaSink(media_sink) == 0);

    active = SDL_TRUE;
    while (active == SDL_TRUE)
    {
        if (SDL_PollEvent(&event) > 0)
        {
            active = handle_event(&event);
        }
    }

    SDL_assert(SDL_StopMediaSink(media_sink) == 0);

    SDL_DestroyMediaSink(media_sink);

    SDL_GL_DeleteContext(gl_context);

    SDL_DestroyWindow(window);

    SDL_Quit();
}

SDL_bool handle_event(const SDL_Event *event)
{
    SDL_bool result;
    result = SDL_TRUE;
    switch (event->type)
    {
    case SDL_QUIT:
        result = SDL_FALSE;
        break;
    case SDL_KEYDOWN:
        if(event->key.keysym.scancode == SDL_SCANCODE_ESCAPE)
        {
            result = SDL_FALSE;
        }
        break;
    default:
        break;
    }
    return result;
}