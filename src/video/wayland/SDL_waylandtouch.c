/*
  Simple DirectMedia Layer - QtWayland Touch Input Support
  Copyright (C) 2013 Jolla Ltd.
  Contact: Thomas Perl <thomas.perl@jollamobile.com>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "SDL_waylandtouch.h"

#include "SDL_log.h"
#include "../../events/SDL_touch_c.h"

#include "wayland-qt-touch-extension.h"

struct SDL_WaylandTouch {
    struct qt_touch_extension *touch_extension;
};


/**
 * Qt TouchPointState
 * adapted from qtbase/src/corelib/global/qnamespace.h
 **/
enum QtWaylandTouchPointState {
    QtWaylandTouchPointPressed    = 0x01,
    QtWaylandTouchPointMoved      = 0x02,
    /*
    Never sent by the server:
    QtWaylandTouchPointStationary = 0x04,
    */
    QtWaylandTouchPointReleased   = 0x08,
};

static void
touch_handle_touch(void *data,
        struct qt_touch_extension *qt_touch_extension,
        uint32_t time,
        uint32_t id,
        uint32_t state,
        int32_t x,
        int32_t y,
        int32_t normalized_x,
        int32_t normalized_y,
        int32_t width,
        int32_t height,
        uint32_t pressure,
        int32_t velocity_x,
        int32_t velocity_y,
        uint32_t flags,
        struct wl_array *rawdata)
{
    /**
     * Event is assembled in QtWayland in TouchExtensionGlobal::postTouchEvent
     * (src/compositor/wayland_wrapper/qwltouch.cpp)
     **/

    float FIXED_TO_FLOAT = 1. / 10000.;
    float xf = FIXED_TO_FLOAT * x;
    float yf = FIXED_TO_FLOAT * y;

    float PRESSURE_TO_FLOAT = 1. / 255.;
    float pressuref = PRESSURE_TO_FLOAT * pressure;

    uint32_t touchState = state & 0xFFFF;
    /*
    Other fields that are sent by the server (qwltouch.cpp),
    but not used at the moment can be decoded in this way:

    uint32_t sentPointCount = state >> 16;
    uint32_t touchFlags = flags & 0xFFFF;
    uint32_t capabilities = flags >> 16;
    */

    SDL_TouchID deviceId = 0;
    if (!SDL_GetTouch(deviceId)) {
        if (SDL_AddTouch(deviceId, "qt_touch_extension") < 0) {
             SDL_Log("error: can't add touch %s, %d", __FILE__, __LINE__);
        }
    }

    switch (touchState) {
        case QtWaylandTouchPointPressed:
        case QtWaylandTouchPointReleased:
            SDL_SendTouch(deviceId, (SDL_FingerID)id,
                    (touchState == QtWaylandTouchPointPressed) ? SDL_TRUE : SDL_FALSE,
                    xf, yf, pressuref);
            break;
        case QtWaylandTouchPointMoved:
            SDL_SendTouchMotion(deviceId, (SDL_FingerID)id, xf, yf, pressuref);
            break;
        default:
            /* Should not happen */
            break;
    }
}

static void
touch_handle_configure(void *data,
        struct qt_touch_extension *qt_touch_extension,
        uint32_t flags)
{
}

static const struct qt_touch_extension_listener touch_listener = {
    touch_handle_touch,
    touch_handle_configure,
};

void
Wayland_touch_create(SDL_WaylandData *data, uint32_t id)
{
    if (data->touch) {
        Wayland_touch_destroy(data);
    }

    data->touch = malloc(sizeof(struct SDL_WaylandTouch));

    struct SDL_WaylandTouch *touch = data->touch;
    touch->touch_extension = wl_registry_bind(data->registry, id, &qt_touch_extension_interface, 1);
    qt_touch_extension_add_listener(touch->touch_extension, &touch_listener, data);
}

void
Wayland_touch_destroy(SDL_WaylandData *data)
{
    if (data->touch) {
        struct SDL_WaylandTouch *touch = data->touch;
        if (touch->touch_extension) {
            qt_touch_extension_destroy(touch->touch_extension);
        }

        free(data->touch);
        data->touch = NULL;
    }
}

