/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga
    Copyright (C) 2013 Sam Spilsbury

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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>


#include "../SDL_sysvideo.h"

#include "SDL_config.h"
#include "SDL_mouse.h"
#include "../../events/SDL_mouse_c.h"
#include "SDL_waylandvideo.h"
#include "SDL_waylandevents_c.h"

#include "SDL_assert.h"

#if SDL_VIDEO_DRIVER_WAYLAND

typedef struct {
    struct wl_buffer   *buffer;
    struct wl_surface  *surface;

    int                hot_x, hot_y;

    /* Either a preloaded cursor, or one we created ourselves */
    struct wl_cursor   *cursor;
    void               *shm_data;
} Wayland_CursorData;

/*
 * Create a new, unique, anonymous file of the given size, and
 * return the file descriptor for it. The file descriptor is set
 * CLOEXEC. The file is immediately suitable for mmap()'ing
 * the given size at offset zero.
 *
 * The file should not have a permanent backing store like a disk,
 * but may have if XDG_RUNTIME_DIR is not properly implemented in OS.
 *
 * The file name is deleted from the file system.
 *
 * The file is suitable for buffer sharing between processes by
 * transmitting the file descriptor over Unix sockets using the
 * SCM_RIGHTS methods.
 *
 * copied from os_create_anonymouse_file in weston
 *
 * Copyright (c) 2012 Collabora Ltd.
 */
static int
create_tmpfile_cloexec(char *tmpname)
{
    int fd;

    fd = mkostemp(tmpname, O_CLOEXEC);
    if (fd >= 0)
        unlink(tmpname);

    return fd;
}

int
os_create_anonymous_file(off_t size)
{
    static const char template[] = "/sdl-shared-XXXXXX";
    const char *path;
    char *name;
    int fd;

    path = getenv("XDG_RUNTIME_DIR");
    if (!path) {
        errno = ENOENT;
        return -1;
    }

    name = malloc(strlen(path) + sizeof(template));
    if (!name)
        return -1;

    strcpy(name, path);
    strcat(name, template);

    fd = create_tmpfile_cloexec(name);

    free(name);

    if (fd < 0)
        return -1;

    if (ftruncate(fd, size) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

static void
mouse_buffer_release(void *data, struct wl_buffer *buffer)
{
}

static const struct wl_buffer_listener mouse_buffer_listener = {
    mouse_buffer_release
};

static int
create_buffer_from_shm(Wayland_CursorData *d,
                       int width,
                       int height,
                       uint32_t format)
{
    SDL_VideoDevice *vd = SDL_GetVideoDevice();
    SDL_WaylandData *data = (SDL_WaylandData *) vd->driverdata;

    int stride = width * 4;
    int size = stride * height;

    int shm_fd;

    shm_fd = os_create_anonymous_file(size);
    if (shm_fd < 0)
    {
        fprintf(stderr, "creating mouse cursor buffer failed!\n");
        return -1;
    }

    d->shm_data = mmap(NULL,
                       size,
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED,
                       shm_fd,
                       0);
    if (data == MAP_FAILED) {
        d->shm_data = NULL;
        fprintf (stderr, "mmap () failed\n");
        close (shm_fd);
    }

    struct wl_shm_pool *shm_pool = wl_shm_create_pool(data->shm,
                                                      shm_fd,
                                                      size);
    d->buffer = wl_shm_pool_create_buffer(shm_pool,
                                          0,
                                          width,
                                          height,
                                          stride,
                                          format);
    wl_buffer_add_listener(d->buffer,
                           &mouse_buffer_listener,
                           d);

    wl_shm_pool_destroy (shm_pool);
    close (shm_fd);

    return 0;
}

static SDL_Cursor *
Wayland_CreateCursor(SDL_Surface *surface, int hot_x, int hot_y)
{
    SDL_Cursor *cursor;

    cursor = calloc(1, sizeof (*cursor));
    if (cursor) {
        SDL_VideoDevice *vd = SDL_GetVideoDevice ();
        SDL_WaylandData *wd = (SDL_WaylandData *) vd->driverdata;
        Wayland_CursorData *data = calloc (1, sizeof (Wayland_CursorData));
        cursor->driverdata = (void *) data;

        /* Assume ARGB8888 */
        SDL_assert(surface->format->format == SDL_PIXELFORMAT_ARGB8888);
        SDL_assert(surface->pitch == surface->w * 4);

        /* Allocate shared memory buffer for this cursor */
        if (create_buffer_from_shm (data,
                                    surface->w,
                                    surface->h,
                                    WL_SHM_FORMAT_XRGB8888) < 0)
        {
            free (cursor->driverdata);
            free (cursor);
            return NULL;
        }

        SDL_memcpy(data->shm_data,
                   surface->pixels,
                   surface->h * surface->pitch);

        data->surface = wl_compositor_create_surface(wd->compositor);
        wl_surface_attach(data->surface,
                          data->buffer,
                          0,
                          0);
        wl_surface_damage(data->surface,
                          0,
                          0,
                          surface->w,
                          surface->h);
        wl_surface_commit(data->surface);

        data->hot_x = hot_x;
        data->hot_y = hot_y;
    }

    return cursor;
}

static SDL_Cursor *
CreateCursorFromWlCursor(SDL_WaylandData *d, struct wl_cursor *wlcursor)
{
    SDL_Cursor *cursor;

    cursor = calloc(1, sizeof (*cursor));
    if (cursor) {
        Wayland_CursorData *data = calloc (1, sizeof (Wayland_CursorData));
        cursor->driverdata = (void *) data;

        /* The wl_buffer here will be destroyed from wl_cursor_theme_destroy
         * if we are fetching this from a wl_cursor_theme, so don't store a
         * reference to it here */
        data->buffer = NULL;
        data->surface = wl_compositor_create_surface(d->compositor);
        wl_surface_attach(data->surface,
                          wl_cursor_image_get_buffer(wlcursor->images[0]),
                          0,
                          0);
        wl_surface_damage(data->surface,
                          0,
                          0,
                          wlcursor->images[0]->width,
                          wlcursor->images[0]->height);
        data->hot_x = wlcursor->images[0]->hotspot_x;
        data->hot_y = wlcursor->images[0]->hotspot_y;
        data->cursor= wlcursor;
    } else {
        SDL_OutOfMemory ();
    }

    return cursor;
}

static SDL_Cursor *
Wayland_CreateDefaultCursor()
{
    SDL_VideoDevice *device = SDL_GetVideoDevice();
    SDL_WaylandData *data = device->driverdata;

    return CreateCursorFromWlCursor (data,
                                     wl_cursor_theme_get_cursor(data->cursor_theme,
                                                                "left_ptr"));
}

static SDL_Cursor *
Wayland_CreateSystemCursor(SDL_SystemCursor id)
{
    SDL_VideoDevice *vd = SDL_GetVideoDevice();
    SDL_WaylandData *d = vd->driverdata;

    struct wl_cursor *cursor = NULL;

    switch(id)
    {
    default:
        SDL_assert(0);
        return NULL;
    case SDL_SYSTEM_CURSOR_ARROW:
        cursor = wl_cursor_theme_get_cursor(d->cursor_theme, "left_ptr");
        break;
    case SDL_SYSTEM_CURSOR_IBEAM:
        cursor = wl_cursor_theme_get_cursor(d->cursor_theme, "xterm");
        break;
    case SDL_SYSTEM_CURSOR_WAIT:
        cursor = wl_cursor_theme_get_cursor(d->cursor_theme, "wait");
        break;
    case SDL_SYSTEM_CURSOR_CROSSHAIR:
        cursor = wl_cursor_theme_get_cursor(d->cursor_theme, "hand1");
        break;
    case SDL_SYSTEM_CURSOR_WAITARROW:
        cursor = wl_cursor_theme_get_cursor(d->cursor_theme, "wait");
        break;
    case SDL_SYSTEM_CURSOR_SIZENWSE:
        cursor = wl_cursor_theme_get_cursor(d->cursor_theme, "hand1");
        break;
    case SDL_SYSTEM_CURSOR_SIZENESW:
        cursor = wl_cursor_theme_get_cursor(d->cursor_theme, "hand1");
        break;
    case SDL_SYSTEM_CURSOR_SIZEWE:
        cursor = wl_cursor_theme_get_cursor(d->cursor_theme, "hand1");
        break;
    case SDL_SYSTEM_CURSOR_SIZENS:
        cursor = wl_cursor_theme_get_cursor(d->cursor_theme, "hand1");
        break;
    case SDL_SYSTEM_CURSOR_SIZEALL:
        cursor = wl_cursor_theme_get_cursor(d->cursor_theme, "hand1");
        break;
    case SDL_SYSTEM_CURSOR_NO:
        cursor = wl_cursor_theme_get_cursor(d->cursor_theme, "xterm");
        break;
    case SDL_SYSTEM_CURSOR_HAND:
        cursor = wl_cursor_theme_get_cursor(d->cursor_theme, "hand1");
        break;
    }

    SDL_Cursor *sdl_cursor = CreateCursorFromWlCursor (d, cursor);

    return sdl_cursor;
}

static void
Wayland_FreeCursor(SDL_Cursor *cursor)
{
    if (!cursor)
        return;

    Wayland_CursorData *d = cursor->driverdata;

    /* Probably not a cursor we own */
    if (!d)
        return;

    if (d->buffer)
        wl_buffer_destroy(d->buffer);

    if (d->surface)
        wl_surface_destroy(d->surface);

    /* Not sure what's meant to happen to shm_data */
    free (cursor->driverdata);
    SDL_free(cursor);
}

static int
Wayland_ShowCursor(SDL_Cursor *cursor)
{
    SDL_VideoDevice *vd = SDL_GetVideoDevice();
    SDL_WaylandData *d = vd->driverdata;

    struct wl_pointer *pointer = wl_seat_get_pointer(d->seat);

    if (!pointer)
        return;

    if (cursor)
    {
        Wayland_CursorData *data = cursor->driverdata;

        wl_pointer_set_cursor (pointer, 0 /* Not sure about this */,
                               data->surface,
                               data->hot_x,
                               data->hot_y);
    }
    else
    {
        wl_pointer_set_cursor (pointer, 0 /* Not sure about this */,
                               NULL,
                               0,
                               0);
    }
}

static void
Wayland_WarpMouse(SDL_Window *window, int x, int y)
{
    SDL_Unsupported();
    return;
}

static int
Wayland_SetRelativeMouseMode(SDL_bool enabled)
{
    SDL_Unsupported();
    return -1;
}

void
Wayland_InitMouse(void)
{
    SDL_Mouse *mouse = SDL_GetMouse();

    mouse->CreateCursor = Wayland_CreateCursor;
    mouse->CreateSystemCursor = Wayland_CreateSystemCursor;
//    mouse->ShowCursor = Wayland_ShowCursor;
    mouse->FreeCursor = Wayland_FreeCursor;
    mouse->WarpMouse = Wayland_WarpMouse;
    mouse->SetRelativeMouseMode = Wayland_SetRelativeMouseMode;

    SDL_SetDefaultCursor(Wayland_CreateDefaultCursor());
}

void
Wayland_FiniMouse(void)
{
    /* This effectively assumes that nobody else
     * touches SDL_Mouse which is effectively
     * a singleton */

    SDL_Mouse *mouse = SDL_GetMouse();

    /* Free the current cursor if not the same pointer as
     * the default cursor */
    if (mouse->def_cursor != mouse->cur_cursor)
        Wayland_FreeCursor (mouse->cur_cursor);

    Wayland_FreeCursor (mouse->def_cursor);
    mouse->def_cursor = NULL;
    mouse->cur_cursor = NULL;

    mouse->CreateCursor =  NULL;
    mouse->CreateSystemCursor = NULL;
    mouse->ShowCursor = NULL;
    mouse->FreeCursor = NULL;
    mouse->WarpMouse = NULL;
    mouse->SetRelativeMouseMode = NULL;
}
#endif
