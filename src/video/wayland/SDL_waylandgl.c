/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga

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

#include "SDL_waylandgl.h"
#include "SDL_waylandwindow.h"
#include <dlfcn.h>

/*
 * FIXME: Either extend the wayland protocol with a flipped attribute for a
 *        surface or better add a wayland egl_platform to mesa.
*/
void flip_egl_image(_THIS, SDL_Window * _window)
{
    SDL_WaylandWindow *window = _window->driverdata;
    SDL_WaylandData *data = _this->driverdata;

    static struct wl_visual *visual;
    static struct wl_buffer *buffer = NULL, *tmp_buffer;
    static GLuint color_rbo, tmp_color_rbo;
    static EGLImageKHR *image = NULL, *tmp_image;

    GLuint texture;
    GLboolean texture_2d_is_enabled;

    EGLint name, stride;
    static EGLint attribs[] = {
		EGL_WIDTH,	0,
		EGL_HEIGHT,	0,
		EGL_DRM_BUFFER_FORMAT_MESA, EGL_DRM_BUFFER_FORMAT_ARGB32_MESA,
		EGL_DRM_BUFFER_USE_MESA,    EGL_DRM_BUFFER_USE_SCANOUT_MESA,
		EGL_NONE
	};

    static const GLint verts[8]      = { -1, -1,  1, -1,  -1, 1,  1, 1 };
    static const GLint tex_coords[8] = {  0,  0,  1,  0,   0, 1,  1, 1 };

    if (buffer == NULL && image == NULL) {
    	visual = wl_display_get_premultiplied_argb_visual(data->display);
        attribs[1] = window->sdlwindow->w;
        attribs[3] = window->sdlwindow->h;
        image = eglCreateDRMImageMESA(data->edpy, attribs);
        eglExportDRMImageMESA(data->edpy, image, &name, NULL, &stride);
        buffer = wl_drm_create_buffer(data->drm, name,
					     window->sdlwindow->w, window->sdlwindow->h,
					     stride, visual);

        glGenRenderbuffers(1, &color_rbo);
    }

    glBindRenderbuffer(GL_RENDERBUFFER_EXT, color_rbo);
    glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER_EXT, image);

    glGenTextures(1, &texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    texture_2d_is_enabled = glIsEnabled(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_2D);

    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, window->image[window->current]);
    
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER_EXT,
                  GL_COLOR_ATTACHMENT0_EXT,
                  GL_RENDERBUFFER_EXT,
                  color_rbo);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glScalef(1.0, -1.0, 1.0);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2, GL_INT, 0, verts);
    glTexCoordPointer(2, GL_INT, 0, tex_coords);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    if (!texture_2d_is_enabled)
        glDisable(GL_TEXTURE_2D);
    glDeleteTextures(1, &texture);

    tmp_image = window->image[window->current];
    tmp_buffer = window->buffer[window->current];
    tmp_color_rbo = window->color_rbo[window->current];

    window->image[window->current] = image;
    window->buffer[window->current] = buffer;
    window->color_rbo[window->current] = color_rbo;

    image = tmp_image;
    buffer = tmp_buffer;
    color_rbo = tmp_color_rbo;
}

void Wayland_GL_SwapWindow(_THIS, SDL_Window * window)
{
    SDL_WaylandWindow *data = window->driverdata;

    glFlush();
    GLfloat clear_color[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clear_color);

    flip_egl_image(_this, window);

    data->current ^= 1;

    glFlush();
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER_EXT,
                  GL_COLOR_ATTACHMENT0_EXT,
                  GL_RENDERBUFFER_EXT,
                  data->color_rbo[data->current]);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(clear_color[0], clear_color[1],
                 clear_color[2], clear_color[3]);

    wl_surface_attach(data->surface, data->buffer[data->current ^ 1]);
    wl_surface_damage(data->surface, 0, 0, window->w, window->h);
}

int
Wayland_GL_MakeCurrent(_THIS, SDL_Window *window, SDL_GLContext context)
{
    SDL_WaylandData *data = _this->driverdata;
    SDL_WaylandWindow *wind = (SDL_WaylandWindow *) window->driverdata;
    int i;

    if (!eglMakeCurrent(data->edpy,EGL_NO_SURFACE,
                        EGL_NO_SURFACE,wind->context)) {
        SDL_SetError("Unable to make EGL context current");
        return -1;
    }

    if (!data->fbo_generated) {
    	glGenFramebuffers(1, &data->fbo);
        data->fbo_generated = 1;
    }
	glBindFramebuffer(GL_FRAMEBUFFER_EXT, data->fbo);

    if (!wind->rbos_generated) {
        glGenRenderbuffers(1, &wind->depth_rbo);
        glGenRenderbuffers(2, wind->color_rbo);
        wind->rbos_generated = 1;
    }

    glBindRenderbuffer(GL_RENDERBUFFER_EXT, wind->depth_rbo);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER_EXT,
                              GL_DEPTH_ATTACHMENT_EXT,
                              GL_RENDERBUFFER_EXT,
                              wind->depth_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER_EXT,
                          GL_DEPTH_COMPONENT,
                          wind->sdlwindow->w, wind->sdlwindow->h);

    for (i = 0; i < 2; ++i) {
        glBindRenderbuffer(GL_RENDERBUFFER_EXT, wind->color_rbo[i]);
        glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER_EXT,
                               wind->image[i]);
    }

	wind->current = 0;
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER_EXT,
				  GL_COLOR_ATTACHMENT0_EXT,
				  GL_RENDERBUFFER_EXT,
				  wind->color_rbo[wind->current]);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE) {
        SDL_SetError("Framebuffer Incomplete.");
        return -1;
    }

    return 0;
}

int
Wayland_GL_LoadLibrary(_THIS, const char *path)
{
    SDL_WaylandData *data = _this->driverdata;


    /* Unload the old driver and reset the pointers */
    Wayland_GL_UnloadLibrary(_this);

    
    data->edpy = eglGetDRMDisplayMESA(data->drm_fd);

    int major, minor;
    if (!eglInitialize(data->edpy, &major, &minor)) {
        SDL_SetError("Failed to initialize display.");
        return -1;
    }

    eglBindAPI(EGL_OPENGL_API);

    data->fbo_generated = 0;

    return 0;
}

void *
Wayland_GL_GetProcAddress(_THIS, const char *proc)
{
    static char procname[1024];
    void *handle;
    void *retval;

    handle = _this->gl_config.dll_handle;
	retval = eglGetProcAddress(proc);
	if (retval) {
		return retval;
	}

#if defined(__OpenBSD__) && !defined(__ELF__)
#undef dlsym(x,y);
#endif
    retval = dlsym(handle, proc);
    if (!retval && strlen(proc) <= 1022) {
        procname[0] = '_';
        strcpy(procname + 1, proc);
        retval = dlsym(handle, procname);
    }
    return retval;
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
}

SDL_GLContext
Wayland_GL_CreateContext(_THIS, SDL_Window * window)
{
    SDL_WaylandData *data = _this->driverdata;
    SDL_WaylandWindow *wind = (SDL_WaylandWindow *) window->driverdata;

    wind->context = eglCreateContext(data->edpy, NULL, EGL_NO_CONTEXT, NULL);


    if (wind->context == EGL_NO_CONTEXT) {
        SDL_SetError("Could not create EGL context");
        return NULL;
    }

    Wayland_GL_MakeCurrent(_this, window, NULL);

    return wind->context;
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

/* vi: set ts=4 sw=4 expandtab: */
