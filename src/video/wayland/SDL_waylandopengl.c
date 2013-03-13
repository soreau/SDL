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

#include "SDL_waylandopengl.h"
#include "SDL_waylandwindow.h"
#include "SDL_waylandevents_c.h"

#include <dlfcn.h>

void
Wayland_GL_SwapWindow(_THIS, SDL_Window *window)
{
    SDL_WaylandWindow *wind = window->driverdata;
    SDL_WaylandData *data = _this->driverdata;

    eglSwapBuffers(data->edpy, wind->esurf);

    wayland_schedule_write(data);
}

int
Wayland_GL_LoadLibrary(_THIS, const char *path)
{
    /* FIXME: dlopen the library here? */
    SDL_WaylandData *data = _this->driverdata;
    int major, minor;
    EGLint num_config;

    EGLenum renderable_type = 0;
    EGLenum rendering_api = 0;

#if SDL_VIDEO_RENDER_OGL_ES2
    renderable_type = EGL_OPENGL_ES2_BIT;
    rendering_api = EGL_OPENGL_ES_API;
#elif SDL_VIDEO_RENDER_OGL_ES
    renderable_type = EGL_OPENGL_ES_BIT;
    rendering_api = EGL_OPENGL_ES_API;
#elif SDL_VIDEO_RENDER_OGL
    renderable_type = EGL_OPENGL_BIT;
    rendering_api = EGL_OPENGL_API;
#endif

    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 0,
        EGL_GREEN_SIZE, 0,
        EGL_BLUE_SIZE, 0,
        EGL_DEPTH_SIZE, 0,
        EGL_ALPHA_SIZE, 0,
        EGL_RENDERABLE_TYPE, renderable_type,
        EGL_NONE
    };

    config_attribs[ 3] = _this->gl_config.red_size;
    config_attribs[ 5] = _this->gl_config.green_size;
    config_attribs[ 7] = _this->gl_config.blue_size;
    config_attribs[ 9] = _this->gl_config.depth_size;
    config_attribs[11] = _this->gl_config.alpha_size;

    data->edpy = eglGetDisplay(data->display);

    if (!eglInitialize(data->edpy, &major, &minor)) {
        SDL_SetError("failed to initialize display");
        return -1;
    }

    eglBindAPI(rendering_api);

    if (!eglChooseConfig(data->edpy, config_attribs,
                         &data->econf, 1, &num_config)) {
        SDL_SetError("failed to choose a config");
        return -1;
    }
    if (num_config != 1) {
        SDL_SetError("failed to choose a config");
        return -1;
    }
    Wayland_PumpEvents(_this);

    wayland_schedule_write(data);

    return 0;
}

void *
Wayland_GL_GetProcAddress(_THIS, const char *proc)
{
    return eglGetProcAddress(proc);
}

void
Wayland_GL_UnloadLibrary(_THIS)
{
    SDL_WaylandData *data = _this->driverdata;

    if (_this->gl_config.driver_loaded) {
        eglTerminate(data->edpy);

        dlclose(_this->gl_config.dll_handle);

        _this->gl_config.dll_handle = NULL;
        _this->gl_config.driver_loaded = 0;
    }

    wayland_schedule_write(data);
}

SDL_GLContext
Wayland_GL_CreateContext(_THIS, SDL_Window *window)
{
    SDL_WaylandData *data = _this->driverdata;

    const EGLint *attribs = NULL;
    int    client_version = 0;

#if SDL_VIDEO_RENDER_OGL_ES2 
    client_version = 2;
#elif SDL_VIDEO_RENDER_OGL_ES
    client_version = 1;
#endif

#if SDL_VIDEO_RENDER_OGL_ES2 || SDL_VIDEO_RENDER_OGL_ES
    const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, client_version,
        EGL_NONE
    };

    attribs = context_attribs;
#endif


    data->context = eglCreateContext(data->edpy, data->econf,
                                     EGL_NO_CONTEXT, attribs);


    if (data->context == EGL_NO_CONTEXT) {
        SDL_SetError("Could not create EGL context");
        return NULL;
    }

    Wayland_GL_MakeCurrent(_this, window, NULL);

    wayland_schedule_write(data);

    return data->context;
}

int
Wayland_GL_MakeCurrent(_THIS, SDL_Window *window, SDL_GLContext context)
{
    SDL_WaylandData *data = _this->driverdata;
    SDL_WaylandWindow *wind;
    EGLSurface *surf = NULL;
    EGLContext *ctx = NULL;

    if (window) {
        wind = window->driverdata;
        surf = wind->esurf;
        ctx = data->context;
    }

    if (!eglMakeCurrent(data->edpy, surf, surf, ctx)) {
        SDL_SetError("Unable to make EGL context current");
        return -1;
    }

    wayland_schedule_write(data);

    return 0;
}

int
Wayland_GL_SetSwapInterval(_THIS, int interval)
{
    return 0;
}

int
Wayland_GL_GetSwapInterval(_THIS)
{
    return 0;
}

void
Wayland_GL_DeleteContext(_THIS, SDL_GLContext context)
{
    SDL_WaylandData *data = _this->driverdata;

    eglMakeCurrent(data->edpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglTerminate(data->edpy);
    eglReleaseThread();

    wayland_schedule_write(data);
}

/* vi: set ts=4 sw=4 expandtab: */
