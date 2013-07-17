/* 
 * Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
 * Contact: http://www.qt-project.org/legal
 * 
 * This file is part of the plugins of the Qt Toolkit.
 * 
 * $QT_BEGIN_LICENSE:BSD$
 * You may use this file under the terms of the BSD license as follows:
 * 
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 * * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
 * of its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 * 
 * $QT_END_LICENSE$
 */

#ifndef WINDOWMANAGER_CLIENT_PROTOCOL_H
#define WINDOWMANAGER_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct qt_windowmanager;

extern const struct wl_interface qt_windowmanager_interface;

struct qt_windowmanager_listener {
	/**
	 * hints - (none)
	 * @show_is_fullscreen: (none)
	 */
	void (*hints)(void *data,
		      struct qt_windowmanager *qt_windowmanager,
		      int32_t show_is_fullscreen);
	/**
	 * quit - (none)
	 */
	void (*quit)(void *data,
		     struct qt_windowmanager *qt_windowmanager);
};

static inline int
qt_windowmanager_add_listener(struct qt_windowmanager *qt_windowmanager,
			      const struct qt_windowmanager_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) qt_windowmanager,
				     (void (**)(void)) listener, data);
}

#define QT_WINDOWMANAGER_OPEN_URL	0

static inline void
qt_windowmanager_set_user_data(struct qt_windowmanager *qt_windowmanager, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) qt_windowmanager, user_data);
}

static inline void *
qt_windowmanager_get_user_data(struct qt_windowmanager *qt_windowmanager)
{
	return wl_proxy_get_user_data((struct wl_proxy *) qt_windowmanager);
}

static inline void
qt_windowmanager_destroy(struct qt_windowmanager *qt_windowmanager)
{
	wl_proxy_destroy((struct wl_proxy *) qt_windowmanager);
}

static inline void
qt_windowmanager_open_url(struct qt_windowmanager *qt_windowmanager, uint32_t remaining, const char *url)
{
	wl_proxy_marshal((struct wl_proxy *) qt_windowmanager,
			 QT_WINDOWMANAGER_OPEN_URL, remaining, url);
}

#ifdef  __cplusplus
}
#endif

#endif
