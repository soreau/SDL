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

#include "../SDL_sysvideo.h"

#include "SDL_waylandwindow.h"
#include "SDL_waylandvideo.h"

static void
handle_ping(void *data, struct wl_shell_surface *shell_surface,
            uint32_t serial)
{
    wl_shell_surface_pong(shell_surface, serial);
}

static void
handle_configure(void *data, struct wl_shell_surface *shell_surface,
                 uint32_t edges, int32_t width, int32_t height)
{
}

static void
handle_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
}

static const struct wl_shell_surface_listener shell_surface_listener = {
    handle_ping,
    handle_configure,
    handle_popup_done
};

SDL_bool
Wayland_GetWindowWMInfo(_THIS, SDL_Window * window, SDL_SysWMinfo * info)
{
    SDL_WaylandWindow *data = (SDL_WaylandWindow *) window->driverdata;

    info->info.wl.display = data->waylandData->display;
    info->info.wl.surface = data->surface;
    info->info.wl.shell_surface = data->shell_surface;
    info->subsystem = SDL_SYSWM_WAYLAND;

    return SDL_TRUE;
}

void Wayland_ShowWindow(_THIS, SDL_Window *window)
{
    SDL_WaylandWindow *wind = window->driverdata;

    if (window->flags & SDL_WINDOW_FULLSCREEN)
        wl_shell_surface_set_fullscreen(wind->shell_surface,
                                        WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT,
                                        0, NULL);
    else
        wl_shell_surface_set_toplevel(wind->shell_surface);

    wayland_schedule_write(_this->driverdata);
}

void
Wayland_SetWindowFullscreen(_THIS, SDL_Window * window,
                            SDL_VideoDisplay * _display, SDL_bool fullscreen)
{
    SDL_WaylandWindow *wind = window->driverdata;

    if (fullscreen)
        wl_shell_surface_set_fullscreen(wind->shell_surface,
                                        WL_SHELL_SURFACE_FULLSCREEN_METHOD_SCALE,
                                        0, NULL);
    else
        wl_shell_surface_set_toplevel(wind->shell_surface);

    wayland_schedule_write(_this->driverdata);
}

int Wayland_CreateWindow(_THIS, SDL_Window *window)
{
    SDL_WaylandWindow *data;
    SDL_WaylandData *c;
    struct wl_region *region;

    data = malloc(sizeof *data);
    if (data == NULL)
        return 0;

    c = _this->driverdata;
    window->driverdata = data;

    if (!(window->flags & SDL_WINDOW_OPENGL)) {
        SDL_GL_LoadLibrary(NULL);
        window->flags |= SDL_WINDOW_OPENGL;
    }

    if (window->x == SDL_WINDOWPOS_UNDEFINED) {
        window->x = 0;
    }
    if (window->y == SDL_WINDOWPOS_UNDEFINED) {
        window->y = 0;
    }

    data->waylandData = c;
    data->sdlwindow = window;

    data->surface =
        wl_compositor_create_surface(c->compositor);
    wl_surface_set_user_data(data->surface, data);
    data->shell_surface = wl_shell_get_shell_surface(c->shell,
                                                     data->surface);

    data->egl_window = wl_egl_window_create(data->surface,
                                            window->w, window->h);
    data->esurf =
        eglCreateWindowSurface(c->edpy, c->econf,
                               data->egl_window, NULL);

    if (data->esurf == EGL_NO_SURFACE) {
        SDL_SetError("failed to create a window surface");
        return -1;
    }

    if (data->shell_surface) {
        wl_shell_surface_set_user_data(data->shell_surface, data);
        wl_shell_surface_add_listener(data->shell_surface,
                                      &shell_surface_listener, data);
    }

    region = wl_compositor_create_region(c->compositor);
    wl_region_add(region, 0, 0, window->w, window->h);
    wl_surface_set_opaque_region(data->surface, region);
    wl_region_destroy(region);

    wayland_schedule_write(c);

    return 0;
}

void Wayland_SetWindowSize(_THIS, SDL_Window * window)
{
    SDL_WaylandData *data = _this->driverdata;
    SDL_WaylandWindow *wind = window->driverdata;
    struct wl_region *region;

    wl_egl_window_resize(wind->egl_window, window->w, window->h, 0, 0);

    region = wl_compositor_create_region(data->compositor);
    wl_region_add(region, 0, 0, window->w, window->h);
    wl_surface_set_opaque_region(wind->surface, region);
    wl_region_destroy(region);
}

void Wayland_DestroyWindow(_THIS, SDL_Window *window)
{
    SDL_WaylandData *data = _this->driverdata;
    SDL_WaylandWindow *wind = window->driverdata;

    window->driverdata = NULL;

    if (data) {
        eglDestroySurface(data->edpy, wind->esurf);
        wl_egl_window_destroy(wind->egl_window);

        if (wind->shell_surface)
            wl_shell_surface_destroy(wind->shell_surface);

        wl_surface_destroy(wind->surface);

        SDL_free(wind);
        wayland_schedule_write(data);
    }
}

/* vi: set ts=4 sw=4 expandtab: */
