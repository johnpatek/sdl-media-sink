#ifndef SDL_MEDIA_SINK
#define SDL_MEDIA_SINK

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

typedef enum
{
    SDL_MEDIA_TYPE_TEST,
    SDL_MEDIA_TYPE_RTSP,
    SDL_MEDIA_TYPE_FILE,
} SDL_MediaType;

typedef struct SDL_Framerate {
    int numerator;
    int denominator;
} SDL_Framerate;

typedef struct SDL_MediaSink SDL_MediaSink;

int SDL_InitMediaSink();

void SDL_QuitMediaSink();

/**
 * Construct a new media sink with for a specified window, GL context, and
 * target subregion.
 *
 * NULL can safely be passed as the `target` parameter.
 *
 * The media sink window and GL context will be used to render frames from
 * a media source to a specified region of the drawable surface.
 *
 * \param window the window where the frames will be rendered
 * \param gl_context the OpenGL context used for drawing
 * \param target the subregion of the window where the frames will
 *               be displayed. NULL target will use the entire window.
 * \returns the media sink that was created or NULL on failure; call
 *          SDL_GetError() for more information.
 */
SDL_MediaSink *SDL_CreateMediaSink(
    SDL_Window *window,
    SDL_GLContext *gl_context,
    SDL_Rect *target);

/**
 * Destroy a media sink and clean up any attached media sources.
 *
 * If `media sink` is NULL, this function will return immediately after setting
 * the SDL error message to "Invalid media sink". See SDL_GetError().
 *
 * \param sink the media sink to destroy
 */
void SDL_DestroyMediaSink(SDL_MediaSink *sink);

/**
 * Attach a media source to a media sink.
 *
 * If `media sink` is NULL, this function will return immediately after setting
 * the SDL error message to "Invalid media sink". See SDL_GetError().
 *
 * \param sink the media sink to destroy
 */
int SDL_AttachMediaSink(SDL_MediaSink *sink, SDL_MediaType media_type, const char * source, const SDL_Framerate *framerate);

int SDL_DetachMediaSink(SDL_MediaSink *sink);

int SDL_PlayMediaSink(SDL_MediaSink *sink);

int SDL_PauseMediaSink(SDL_MediaSink *sink);

int SDL_StopMediaSink(SDL_MediaSink *sink);

#endif