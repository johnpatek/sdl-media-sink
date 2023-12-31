cmake_minimum_required(VERSION 3.0)

project(sdl-media-sink)

option(SDL_MEDIA_SINK_EXAMPLE "Build SDL Media Sink example" OFF)

find_package(PkgConfig REQUIRED)
pkg_search_module(glib REQUIRED IMPORTED_TARGET glib-2.0)
pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_search_module(gstreamer-rtsp REQUIRED IMPORTED_TARGET gstreamer-rtsp-1.0)
pkg_search_module(sdl REQUIRED IMPORTED_TARGET sdl2)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_library(sdlmediasink SHARED sdl_media_sink.h sdl_media_sink.c)

target_include_directories(sdlmediasink PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(sdlmediasink PkgConfig::glib PkgConfig::gstreamer PkgConfig::gstreamer-rtsp PkgConfig::sdl)

if(SDL_MEDIA_SINK_EXAMPLE)
    add_subdirectory(example)
endif()