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

#ifndef _SDL_waylandevents_h
#define _SDL_waylandevents_h

#include "SDL_waylandvideo.h"
#include "SDL_waylandwindow.h"

extern void Wayland_PumpEvents(_THIS);

extern void Wayland_display_add_input(SDL_WaylandData *d, uint32_t id);
extern void Wayland_display_destroy_input(SDL_WaylandData *d);

#endif /* _SDL_waylandevents_h */

/* vi: set ts=4 sw=4 expandtab: */
