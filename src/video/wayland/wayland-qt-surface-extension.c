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

#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"

extern const struct wl_interface qt_extended_surface_interface;
extern const struct wl_interface wl_surface_interface;

static const struct wl_interface *types[] = {
	NULL,
	NULL,
	&qt_extended_surface_interface,
	&wl_surface_interface,
};

static const struct wl_message qt_surface_extension_requests[] = {
	{ "get_extended_surface", "no", types + 2 },
};

WL_EXPORT const struct wl_interface qt_surface_extension_interface = {
	"qt_surface_extension", 1,
	1, qt_surface_extension_requests,
	0, NULL,
};

static const struct wl_message qt_extended_surface_requests[] = {
	{ "update_generic_property", "sa", types + 0 },
	{ "set_content_orientation", "i", types + 0 },
	{ "set_window_flags", "i", types + 0 },
};

static const struct wl_message qt_extended_surface_events[] = {
	{ "onscreen_visibility", "i", types + 0 },
	{ "set_generic_property", "sa", types + 0 },
	{ "close", "", types + 0 },
};

WL_EXPORT const struct wl_interface qt_extended_surface_interface = {
	"qt_extended_surface", 1,
	3, qt_extended_surface_requests,
	3, qt_extended_surface_events,
};

