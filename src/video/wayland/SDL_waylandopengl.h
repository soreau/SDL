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

#ifndef _SDL_waylandopengl_h
#define _SDL_waylandopengl_h

#include "SDL_waylandwindow.h"

extern void Wayland_GL_SwapWindow(_THIS, SDL_Window *window);
extern int Wayland_GL_GetSwapInterval(_THIS);
extern int Wayland_GL_SetSwapInterval(_THIS, int interval);
extern int Wayland_GL_MakeCurrent(_THIS, SDL_Window *window, SDL_GLContext context);
extern SDL_GLContext Wayland_GL_CreateContext(_THIS, SDL_Window *window);
extern int Wayland_GL_LoadLibrary(_THIS, const char *path);
extern void Wayland_GL_UnloadLibrary(_THIS);
extern void *Wayland_GL_GetProcAddress(_THIS, const char *proc);
extern void Wayland_GL_DeleteContext(_THIS, SDL_GLContext context);

#endif /* _SDL_waylandopengl_h */

/* vi: set ts=4 sw=4 expandtab: */
