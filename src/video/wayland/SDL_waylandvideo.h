/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga
    Copyright (C) 2010 Joel Teichroeb <joel@teichroeb.net>
    Copyright (C) 2010-2012 Benjamin Franzke <benjaminfranzke@googlemail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#include "SDL_config.h"

#ifndef _SDL_waylandvideo_h
#define _SDL_waylandvideo_h

#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>

#include "wayland-qt-surface-extension.h"
#include "wayland-qt-windowmanager.h"

#include <EGL/egl.h>

struct xkb_context;
struct SDL_WaylandInput;
struct SDL_WaylandTouch;

typedef struct {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_output *output;
    struct wl_shm *shm;
    struct wl_cursor_theme *cursor_theme;
    struct wl_cursor *default_cursor;
    struct wl_pointer *pointer;
    struct wl_shell *shell;

    struct {
        int32_t x, y, width, height;
    } screen_allocation;

    EGLDisplay edpy;
    EGLContext context;
    EGLConfig econf;

    struct xkb_context *xkb_context;
    struct SDL_WaylandInput *input;
    struct SDL_WaylandTouch *touch;
    struct qt_surface_extension *surface_extension;
    struct qt_windowmanager *windowmanager;

    uint32_t shm_formats;
} SDL_WaylandData;

static inline void
wayland_schedule_write(SDL_WaylandData *data)
{
    wl_display_flush(data->display);
}

#endif /* _SDL_nullvideo_h */

/* vi: set ts=4 sw=4 expandtab: */
