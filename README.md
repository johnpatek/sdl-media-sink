# SDL Media Sink

Display video sources to an SDL window using GStreamer and OpenGL. Currently supports streaming from file sources, RTSP, and `videotestsrc`.

## Usage

The syntax and usage is designed to work with existing SDL code. Any GStreamer or GLib code has been abstracted away in the implementation. The following example outlines the basic usage for constructing a media sink with a `videotestsrc` pipeline:

```c
// initialize SDL and SDL media sink
SDL_Window *window;
SDL_GLContext *gl_context;
SDL_MediaSink *media_sink;

// create window and GL context

media_sink = SDL_CreateMediaSink(window, gl_context, NULL);
SDL_AttachMediaSink(media_sink, SDL_MEDIA_TYPE_TEST, NULL, NULL);
```

## TODO

The following items should be added:
+ audio support
+ cross platform support
+ wayland support