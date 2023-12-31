#include "sdl_media_sink.h"

#include <gst/gst.h>
#include <gst/gl/gl.h>

#ifdef WIN32
#include <windows.h>
#endif

#include <GL/gl.h>

#ifndef WIN32
#include <GL/glx.h>
#include <SDL2/SDL_syswm.h>
#include <gst/gl/x11/gstgldisplay_x11.h>
#endif

#define GST_GL_APP_CONTEXT_TYPE "gst.gl.app_context"

typedef struct SDL_MediaPipelineParameters
{

} SDL_MediaPipelineParameters;

typedef struct SDL_MediaPipeline
{
    GstGLDisplay *gst_gl_display;
    GstGLContext *gst_gl_context;
    GstElement *gst_glimagesink;
    GstElement *gst_pipeline;
    GstBus *gst_bus;
    GstVideoInfo gst_video_info;
} SDL_MediaPipeline;

typedef struct SDL_MediaSink
{
    SDL_Window *window;
    SDL_GLContext *gl_context;
    int width;
    int height;
    SDL_Rect target;
    SDL_MediaPipeline *pipeline;
} SDL_MediaSink;

// media sink helper functions
static SDL_MediaPipeline *SDL_CreateMediaPipeline(
    SDL_Window *window,
    SDL_GLContext *gl_context,
    SDL_MediaType media_type,
    const char *source,
    int width,
    int height,
    int framerate_numerator,
    int framerate_denominator);

static void SDL_DestroyMediaPipeline(
    SDL_MediaPipeline *pipeline);

// GStreamer helper functions
static GstElement *build_rtsp_pipeline(
    GstElement *glimagesink,
    GstCaps *caps,
    const char *const url);
static GstElement *build_file_pipeline(
    GstElement *glimagesink,
    GstCaps *caps,
    const char *const path);
static GstElement *build_test_pipeline(
    GstElement *glimagesink,
    GstCaps *caps);
static gboolean on_client_draw(
    GstElement *glimagesink,
    GstGLContext *context,
    GstSample *sample,
    gpointer data);
static void on_sync_bus(
    GstBus *bus,
    GstMessage *msg,
    gpointer data);

int SDL_InitMediaSink()
{
    // TODO: gst_init_check()
    gst_init(NULL, NULL);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    return 0;
}

void SDL_QuitMediaSink()
{
    gst_deinit();
}

SDL_MediaSink *SDL_CreateMediaSink(
    SDL_Window *window,
    SDL_GLContext *gl_context,
    SDL_Rect *target)
{
    SDL_MediaSink *sink;

    sink = NULL;

    if (window == NULL)
    {
        SDL_SetError("Invalid window");
        goto error;
    }

    if (gl_context == NULL)
    {
        SDL_SetError("Invalid GL context");
        goto error;
    }

    sink = SDL_calloc(1, sizeof(SDL_MediaSink));
    sink->window = window;
    sink->gl_context = gl_context;
    sink->pipeline = NULL;
    SDL_GetWindowSize(window, &sink->width, &sink->height);
    if (target != NULL)
    {
        if (target->x < 0 || target->y < 0 || target->w > sink->width || target->h > sink->height)
        {
            SDL_SetError("Invalid target");
            goto error;
        }
        SDL_memcpy(&sink->target, target, sizeof(SDL_Rect));
    }
    else
    {
        sink->target.x = 0;
        sink->target.y = 0;
        sink->target.w = sink->width;
        sink->target.h = sink->height;
    }

    goto done;
error:
    if (sink != NULL)
    {
        SDL_DestroyMediaSink(sink);
    }
    sink = NULL;
done:
    return sink;
}

void SDL_DestroyMediaSink(SDL_MediaSink *sink)
{
    if (sink != NULL)
    {
        if (sink->pipeline != NULL)
        {
        }
        free(sink);
    }
    else
    {
        SDL_SetError("invalid media sink");
    }
}

int SDL_AttachMediaSink(SDL_MediaSink *sink, SDL_MediaType media_type, const char *source, const SDL_Framerate *framerate)
{
    const SDL_Framerate maximum_framerate = {
        .numerator = 0,
        .denominator = 1,
    };
    SDL_MediaPipeline *pipeline;
    int result;

    result = 0;

    if (sink == NULL)
    {
        SDL_SetError("Invalid media sink");
        goto error;
    }

    if (sink->pipeline != NULL)
    {
        SDL_SetError("Media sink already attached");
        goto error;
    }

    switch (media_type)
    {
    case SDL_MEDIA_TYPE_TEST:
    case SDL_MEDIA_TYPE_FILE:
    case SDL_MEDIA_TYPE_RTSP:
        break;
    default:
        SDL_SetError("Invalid media type");
        goto error;
    }

    // TODO: validate FPS
    if (framerate == NULL)
    {
        framerate = &maximum_framerate;
    }

    sink->pipeline = SDL_CreateMediaPipeline(media_type, source, sink->width, sink->height, framerate->numerator, framerate->denominator);
    if (sink->pipeline == NULL)
    {
        goto error;
    }

    g_signal_connect(G_OBJECT(sink->pipeline->gst_glimagesink), "client-draw", G_CALLBACK(on_client_draw), sink);

    goto done;
error:
    result = 1;
done:
    return result;
}

int SDL_DetachMediaSink(SDL_MediaSink *sink)
{
    int result;

    result = 0;

    if (sink == NULL)
    {
        SDL_SetError("Invalid sink");
        goto error;
    }

    if (sink->pipeline == NULL)
    {
        SDL_SetError("No pipeline attached");
        goto error;
    }

    SDL_DestroyMediaPipeline(sink->pipeline);
    sink->pipeline = NULL;

    goto done;
error:
    result = 1;
done:
    return result;
}

int SDL_PlayMediaSink(SDL_MediaSink *sink)
{
    int result;

    result = 0;

    if (gst_element_set_state(sink->pipeline->gst_pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        SDL_SetError("failed to set pipeline state");
    }

    goto done;
error:
    result = 1;
done:
    return result;
}

int SDL_PauseMediaSink(SDL_MediaSink *sink)
{
    int result;

    result = 0;

    if (gst_element_set_state(sink->pipeline->gst_pipeline, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE)
    {
        SDL_SetError("failed to set pipeline state");
        goto error;
    }

    goto done;
error:
    result = 1;
done:
    return result;
}

int SDL_StopMediaSink(SDL_MediaSink *sink)
{
    int result;

    result = 0;

    if (gst_element_set_state(sink->pipeline->gst_pipeline, GST_STATE_NULL) == GST_STATE_CHANGE_FAILURE)
    {
        SDL_SetError("failed to set pipeline state");
    }

    goto done;
error:
    result = 1;
done:
    return result;
}

SDL_MediaPipeline *SDL_CreateMediaPipeline(
    SDL_Window *window,
    SDL_GLContext *gl_context,
    SDL_MediaType media_type,
    const char *source,
    int width,
    int height,
    int framerate_numerator,
    int framerate_denominator)
{
    const char *caps_format = "video/x-raw(memory:GLMemory),"
                              "format=RGBA,"
                              "width=%d,"
                              "height=%d,"
                              "framerate=%d/%d";
    SDL_MediaPipeline *pipeline;
    SDL_SysWMinfo wm_info;
    char *caps_string;
    GstCaps *caps;

    pipeline = SDL_calloc(1, sizeof(SDL_MediaPipeline));
    caps_string = NULL;

    SDL_GetWindowWMInfo(window, &wm_info);

    (void)SDL_asprintf(&caps_string, caps_format, width, height, framerate_numerator, framerate_denominator);

    caps = gst_caps_from_string(caps_string);
    pipeline->gst_glimagesink = gst_element_factory_make("glimagesink", NULL);
    switch (media_type)
    {
    case SDL_MEDIA_TYPE_TEST:
        pipeline->gst_pipeline = build_test_pipeline(pipeline->gst_glimagesink, caps);
        break;
    case SDL_MEDIA_TYPE_RTSP:
        pipeline->gst_pipeline = build_rtsp_pipeline(pipeline->gst_glimagesink, caps, source);
        break;
    case SDL_MEDIA_TYPE_FILE:
        pipeline->gst_pipeline = build_file_pipeline(pipeline->gst_glimagesink, caps, source);
        break;
    }
    if (pipeline->gst_pipeline == NULL)
    {
        goto error;
    }
    goto done;
error:

done:
    return pipeline;
}

void SDL_DestroyMediaPipeline(
    SDL_MediaPipeline *pipeline)
{
    if (pipeline != NULL)
    {
        SDL_free(pipeline);
    }
}

GstElement *build_rtsp_pipeline(
    GstElement *glimagesink,
    GstCaps *caps,
    const char *const url)
{
    GstElement *pipeline;
    pipeline = gst_pipeline_new("sdl-media-sink");
    return pipeline;
}

GstElement *build_file_pipeline(
    GstElement *glimagesink,
    GstCaps *caps,
    const char *const path)
{
}

GstElement *build_test_pipeline(
    GstElement *glimagesink,
    GstCaps *caps)
{
    const char *error_message;
    GstElement *videotestsrc;
    GstElement *glupload;
    GstElement *pipeline;

    videotestsrc = NULL;
    pipeline = NULL;

    videotestsrc = gst_element_factory_make("videotestsrc", "source");
    if (videotestsrc == NULL)
    {
        error_message = "failed to construct videotestsrc";
        goto error;
    }
    glupload = gst_element_factory_make("videotestsrc", "source");
    if (glupload == NULL)
    {
        error_message = "failed to construct videotestsrc";
        goto error;
    }
    pipeline = gst_pipeline_new("sdl-media-sink");
    if (pipeline == NULL)
    {
        error_message = "failed to construct gstreamer pipeline";
        goto error;
    }

    gst_bin_add_many(GST_BIN(pipeline), videotestsrc, glupload, glimagesink, NULL);

    if (gst_element_link(videotestsrc, glupload) != TRUE)
    {
        error_message = "failed to link glupload and glimagesink";
        goto error;
    }

    if (gst_element_link_filtered(glupload, glimagesink, caps) != TRUE)
    {
        error_message = "failed to link glupload and glimagesink";
        goto error;
    }

    goto done;
error:
    SDL_SetError("Failed to construct test pipeline: %s", "error");
    if (pipeline != NULL)
    {
        gst_object_unref(pipeline);
        pipeline = NULL;
    }
done:
    if (videotestsrc != NULL)
    {
        gst_object_unref(videotestsrc);
    }
    if (glupload != NULL)
    {
        gst_object_unref(glupload);
    }
    return pipeline;
}

gboolean on_client_draw(
    GstElement *glimagesink,
    GstGLContext *context,
    GstSample *sample,
    gpointer data)
{
    const GstMapFlags map_flags = GST_MAP_READ | GST_MAP_GL;
    SDL_MediaSink *const sink = (SDL_MediaSink *const)data;
    SDL_MediaPipeline *const pipeline = sink->pipeline;
    GstBuffer *buffer;
    GstVideoFrame video_frame;
    guint texture;

    buffer = gst_sample_get_buffer(sample);
    gst_video_frame_map(&video_frame, &pipeline->gst_video_info, buffer, map_flags);
    SDL_GL_MakeCurrent(sink->window, sink->gl_context);
    texture = *(guint *)video_frame.data[0];
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-1.0f, 1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(1.0f, 1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-1.0f, -1.0f);
    glEnd();
    SDL_GL_SwapWindow(sink->window);
    SDL_GL_MakeCurrent(sink->window, NULL);
    gst_video_frame_unmap(&video_frame);
    return TRUE;
}

void on_sync_bus(
    GstBus *bus,
    GstMessage *msg,
    gpointer data)
{
    SDL_MediaPipeline *const pipeline = (SDL_MediaPipeline *const)data;
    const gchar *context_type;
    GstContext *context;
    GstStructure *writable_structure;

    context = NULL;

    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_NEED_CONTEXT)
    {
        gst_message_parse_context_type(msg, &context_type);
        if (g_strcmp0(context_type, GST_GL_DISPLAY_CONTEXT_TYPE) == 0)
        {
            context = gst_context_new(GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
            gst_context_set_gl_display(context, pipeline->gst_gl_display);
        }
        else if (g_strcmp0(context_type, GST_GL_APP_CONTEXT_TYPE) == 0)
        {
            context = gst_context_new(GST_GL_APP_CONTEXT_TYPE, TRUE);
            writable_structure = gst_context_writable_structure(context);
            gst_structure_set(writable_structure, "context", pipeline->gst_gl_context, NULL);
        }
    }

    if (context != NULL)
    {
        gst_element_set_context(GST_ELEMENT(msg->src), context);
        gst_context_unref(context);
    }
}