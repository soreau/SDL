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

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../../events/SDL_events_c.h"

#include "SDL_waylandvideo.h"
#include "SDL_waylandevents_c.h"
#include "SDL_waylandwindow.h"
#include "SDL_waylandopengl.h"
#include "SDL_waylandmouse.h"
#include "SDL_waylandtouch.h"

#include <fcntl.h>
#include <xkbcommon/xkbcommon.h>

#define WAYLANDVID_DRIVER_NAME "wayland"

/* Initialization/Query functions */
static int
Wayland_VideoInit(_THIS);

static void
Wayland_GetDisplayModes(_THIS, SDL_VideoDisplay *sdl_display);
static int
Wayland_SetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode);

static void
Wayland_VideoQuit(_THIS);

/* Wayland driver bootstrap functions */
static int
Wayland_Available(void)
{
    const char *envr = SDL_getenv("SDL_VIDEODRIVER");
    return envr && SDL_strcmp(envr, WAYLANDVID_DRIVER_NAME) == 0;
}

static void
Wayland_DeleteDevice(SDL_VideoDevice *device)
{
    SDL_free(device);
}

static SDL_VideoDevice *
Wayland_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;

    /* Initialize all variables that we clean on shutdown */
    device = SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        SDL_OutOfMemory();
        return NULL;
    }

    /* Set the function pointers */
    device->VideoInit = Wayland_VideoInit;
    device->VideoQuit = Wayland_VideoQuit;
    device->SetDisplayMode = Wayland_SetDisplayMode;
    device->GetDisplayModes = Wayland_GetDisplayModes;
    device->GetWindowWMInfo = Wayland_GetWindowWMInfo;

    device->PumpEvents = Wayland_PumpEvents;

    device->GL_SwapWindow = Wayland_GL_SwapWindow;
    device->GL_GetSwapInterval = Wayland_GL_GetSwapInterval;
    device->GL_SetSwapInterval = Wayland_GL_SetSwapInterval;
    device->GL_MakeCurrent = Wayland_GL_MakeCurrent;
    device->GL_CreateContext = Wayland_GL_CreateContext;
    device->GL_LoadLibrary = Wayland_GL_LoadLibrary;
    device->GL_UnloadLibrary = Wayland_GL_UnloadLibrary;
    device->GL_GetProcAddress = Wayland_GL_GetProcAddress;
    device->GL_DeleteContext = Wayland_GL_DeleteContext;

    device->CreateWindow = Wayland_CreateWindow;
    device->ShowWindow = Wayland_ShowWindow;
    device->SetWindowFullscreen = Wayland_SetWindowFullscreen;
    device->SetWindowSize = Wayland_SetWindowSize;
    device->DestroyWindow = Wayland_DestroyWindow;

    device->free = Wayland_DeleteDevice;

    return device;
}

VideoBootStrap Wayland_bootstrap = {
    WAYLANDVID_DRIVER_NAME, "SDL Wayland video driver",
    Wayland_Available, Wayland_CreateDevice
};

static void
display_handle_geometry(void *data,
                        struct wl_output *output,
                        int x, int y,
                        int physical_width,
                        int physical_height,
                        int subpixel,
                        const char *make,
                        const char *model,
                        int transform)

{
    SDL_WaylandData *d = data;

    d->screen_allocation.x = x;
    d->screen_allocation.y = y;
}

static void
display_handle_mode(void *data,
                    struct wl_output *wl_output,
                    uint32_t flags,
                    int width,
                    int height,
                    int refresh)
{
    SDL_WaylandData *d = data;

    if (flags & WL_OUTPUT_MODE_CURRENT) {
        d->screen_allocation.width = width;
        d->screen_allocation.height = height;
    }
}

static const struct wl_output_listener output_listener = {
    display_handle_geometry,
    display_handle_mode
};

static void
shm_handle_format(void *data,
                  struct wl_shm *shm,
                  uint32_t format)
{
    SDL_WaylandData *d = data;

    d->shm_formats |= (1 << format);
}

static const struct wl_shm_listener shm_listener = {
    shm_handle_format
};

static void
windowmanager_hints(void *data, struct qt_windowmanager *qt_windowmanager,
        int32_t show_is_fullscreen)
{
}

static void
windowmanager_quit(void *data, struct qt_windowmanager *qt_windowmanager)
{
    SDL_SendQuit();
}

static const struct qt_windowmanager_listener windowmanager_listener = {
    windowmanager_hints,
    windowmanager_quit,
};

static void
display_handle_global(void *data, struct wl_registry *registry, uint32_t id,
					const char *interface, uint32_t version)
{
    SDL_WaylandData *d = data;

    if (strcmp(interface, "wl_compositor") == 0) {
        d->compositor = wl_registry_bind(d->registry, id, &wl_compositor_interface, 1);
    } else if (strcmp(interface, "wl_output") == 0) {
        d->output = wl_registry_bind(d->registry, id, &wl_output_interface, 1);
        wl_output_add_listener(d->output, &output_listener, d);
    } else if (strcmp(interface, "wl_seat") == 0) {
        Wayland_display_add_input(d, id);
    } else if (strcmp(interface, "wl_shell") == 0) {
        d->shell = wl_registry_bind(d->registry, id, &wl_shell_interface, 1);
    } else if (strcmp(interface, "wl_shm") == 0) {
        d->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
        d->cursor_theme = wl_cursor_theme_load(NULL, 32, d->shm);
        d->default_cursor = wl_cursor_theme_get_cursor(d->cursor_theme, "left_ptr");
        wl_shm_add_listener(d->shm, &shm_listener, d);
    } else if (strcmp(interface, "qt_touch_extension") == 0) {
        Wayland_touch_create(d, id);
    } else if (strcmp(interface, "qt_surface_extension") == 0) {
        d->surface_extension = wl_registry_bind(registry, id,
                &qt_surface_extension_interface, 1);
    } else if (strcmp(interface, "qt_windowmanager") == 0) {
        d->windowmanager = wl_registry_bind(registry, id,
                &qt_windowmanager_interface, 1);
        qt_windowmanager_add_listener(d->windowmanager, &windowmanager_listener, d);
    }
}

static const struct wl_registry_listener registry_listener = {
	display_handle_global
};

int
Wayland_VideoInit(_THIS)
{
    SDL_WaylandData *data;

    data = malloc(sizeof *data);
    if (data == NULL)
        return 0;
    memset(data, 0, sizeof *data);

    _this->driverdata = data;

    data->display = wl_display_connect(NULL);
    if (data->display == NULL) {
        SDL_SetError("Failed to connect to a Wayland display");
        return 0;
    }

    data->registry = wl_display_get_registry(data->display);
    wl_registry_add_listener(data->registry, &registry_listener, data);

    while (data->screen_allocation.width == 0)
        wl_display_dispatch(data->display);

    data->xkb_context = xkb_context_new(0);
    if (!data->xkb_context) {
        SDL_SetError("Failed to create XKB context");
        return 0;
    }

    SDL_VideoDisplay display;
    SDL_DisplayMode mode;

    /* Use a fake 32-bpp desktop mode */
    mode.format = SDL_PIXELFORMAT_RGB888;
    mode.w = data->screen_allocation.width;
    mode.h = data->screen_allocation.height;
    mode.refresh_rate = 0;
    mode.driverdata = NULL;
    SDL_zero(display);
    display.desktop_mode = mode;
    display.current_mode = mode;
    display.driverdata = NULL;
    SDL_AddVideoDisplay(&display);

    Wayland_InitMouse ();

    wayland_schedule_write(data);

    return 0;
}

static void
Wayland_GetDisplayModes(_THIS, SDL_VideoDisplay *sdl_display)
{
    SDL_WaylandData *data = _this->driverdata;
    SDL_DisplayMode mode;

    Wayland_PumpEvents(_this);

    mode.w = data->screen_allocation.width;
    mode.h = data->screen_allocation.height;
    mode.refresh_rate = 0;
    mode.driverdata = NULL;

    mode.format = SDL_PIXELFORMAT_RGB888;
    SDL_AddDisplayMode(sdl_display, &mode);
    mode.format = SDL_PIXELFORMAT_RGBA8888;
    SDL_AddDisplayMode(sdl_display, &mode);
}

static int
Wayland_SetDisplayMode(_THIS, SDL_VideoDisplay *display, SDL_DisplayMode *mode)
{
    return 0;
}

void
Wayland_VideoQuit(_THIS)
{
    SDL_WaylandData *data = _this->driverdata;

    Wayland_FiniMouse ();

    if (data->output)
        wl_output_destroy(data->output);

    Wayland_display_destroy_input(data);

    if (data->xkb_context) {
        xkb_context_unref(data->xkb_context);
        data->xkb_context = NULL;
    }

    if (data->windowmanager)
        qt_windowmanager_destroy(data->windowmanager);

    if (data->surface_extension)
        qt_surface_extension_destroy(data->surface_extension);

    Wayland_touch_destroy(data);

    if (data->shm)
        wl_shm_destroy(data->shm);

    if (data->cursor_theme)
        wl_cursor_theme_destroy(data->cursor_theme);

    if (data->shell)
        wl_shell_destroy(data->shell);

    if (data->compositor)
        wl_compositor_destroy(data->compositor);

    if (data->display) {
        wl_display_flush(data->display);
        wl_display_disconnect(data->display);
    }

    free(data);
    _this->driverdata = NULL;
}

/* vi: set ts=4 sw=4 expandtab: */
