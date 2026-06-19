#include <mc/xlib_wm/wm.h>
#include <mc/xlib_wm/graphics.h>
#include <mc/xlib_wm/key.h>
#include <mc/os/file.h>

#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <mc/data/alloc.h>

#define LOG(FMT, ...) mc_fmt(wm->log, "[WM][XLIB] " FMT "\n", ##__VA_ARGS__)

static MC_Stream *xlib_error_log;

struct MC_TargetWM{
    MC_Stream *log;
    Display *dpy;
    MC_Alloc *arena;

    Atom wm_protocols;
    Atom wm_delete_window;
    Atom net_wm_state;
    Atom net_wm_state_fullscreen;
    Atom net_wm_state_maximized_horz;
    Atom net_wm_state_maximized_vert;
    Atom net_wm_state_hidden;

    Atom clipboard;
    Atom utf8_string;
    Atom selection_property;
};

struct MC_TargetWMWindow{
    Window window_id;
};

struct MC_TargetForeignWindow{
    Window window_id;
};

struct MC_TargetWMEvent{
    XEvent event;
};

static MC_Error init(struct MC_TargetWM *wm, MC_Stream *log, MC_Alloc *arena);
static void destroy(struct MC_TargetWM *wm);
static MC_Error init_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WindowParameters *params);
static void destroy_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window);
static MC_Error create_window_graphic(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, struct MC_Graphics **g);
static MC_Error set_window_title(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Str title);
static MC_Error set_window_state(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WMWindowState state);
static MC_Error set_window_rect(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WMArea area, MC_Rect2IU rect);
static MC_Error focus_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window);
static MC_Error get_window_title(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, char *utf8, size_t cap, size_t *len);
static MC_Error get_window_state(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WMWindowState *state);
static MC_Error get_window_rect(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WMArea area, MC_Rect2IU *rect);
static MC_Error request_events(struct MC_TargetWM *wm, MC_WMEvents events);
static MC_Error get_focused_window(struct MC_TargetWM *wm, uint64_t *identity);
static MC_Error get_hovered_window(struct MC_TargetWM *wm, uint64_t *identity);
static MC_Error get_all_windows(struct MC_TargetWM *wm, uint64_t **identities, size_t *count);
static MC_Error resolve_temporary_identity(struct MC_TargetWM *wm, uint64_t identity, MC_TargetResolvedIdentity *out);
static void heartbeat(struct MC_TargetWM *wm);
static void destroy_foreign_window(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window);
static MC_Error close_foreign_window(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window);
static MC_Error set_foreign_window_title(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_Str title);
static MC_Error set_foreign_window_state(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_WMWindowState state);
static MC_Error set_foreign_window_rect(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_WMArea area, MC_Rect2IU rect);
static MC_Error focus_foreign_window(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window);
static MC_Error get_foreign_window_title(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, char *utf8, size_t cap, size_t *len);
static MC_Error get_foreign_window_state(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_WMWindowState *state);
static MC_Error get_foreign_window_rect(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_WMArea area, MC_Rect2IU *rect);
static MC_Error is_foreign_window_system(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, bool *out);
static MC_WMWindowState read_window_state(struct MC_TargetWM *wm, Window win);
static bool poll_event(struct MC_TargetWM *wm, struct MC_TargetWMEvent *event);
static unsigned translate_event(struct MC_TargetWM *wm, const struct MC_TargetWMEvent *event, MC_TargetIndication indications[MC_WM_MAX_INDICATIONS_PER_EVENT]);

static MC_MouseButton get_mouse_button(int x11_mouse_button);
static MC_Key get_key(const XKeyEvent *xk);

static struct MC_TargetWMWindow *get_window(struct MC_TargetWM *wm, Window xid);

static MC_WMVtab vtab = {
    .name = "X11",
    .wm_size = sizeof(struct MC_TargetWM),
    .window_size = sizeof(struct MC_TargetWMWindow),
    .foreign_window_size = sizeof(struct MC_TargetForeignWindow),
    .event_size = sizeof(struct MC_TargetWMEvent),

    .init = init,
    .destroy = destroy,

    .init_window = init_window,
    .destroy_window = destroy_window,
    .create_window_graphic = create_window_graphic,

    .set_window_title = set_window_title,
    .set_window_state = set_window_state,
    .set_window_rect = set_window_rect,
    .focus_window = focus_window,
    .get_window_title = get_window_title,
    .get_window_state = get_window_state,
    .get_window_rect = get_window_rect,

    .poll_event = poll_event,
    .translate_event = translate_event,

    .request_events = request_events,

    .get_focused_window = get_focused_window,
    .get_hovered_window = get_hovered_window,
    .get_all_windows = get_all_windows,
    .resolve_temporary_identity = resolve_temporary_identity,
    .heartbeat = heartbeat,

    .destroy_foreign_window = destroy_foreign_window,
    .close_foreign_window = close_foreign_window,
    .set_foreign_window_title = set_foreign_window_title,
    .set_foreign_window_state = set_foreign_window_state,
    .set_foreign_window_rect = set_foreign_window_rect,
    .focus_foreign_window = focus_foreign_window,
    .get_foreign_window_title = get_foreign_window_title,
    .get_foreign_window_state = get_foreign_window_state,
    .get_foreign_window_rect = get_foreign_window_rect,
    .is_foreign_window_system = is_foreign_window_system,
};

const MC_WMVtab *mc_xlib_wm_vtab = &vtab;

Display *mc_wm_xlib_get_display(MC_TargetWM *wm){
    return wm->dpy;
}

Window mc_wm_xlib_window_get_xid(MC_TargetWMWindow *window){
    return window->window_id;
}

static int handle_x_error(Display *dpy, XErrorEvent *err){
    char text[256];
    XGetErrorText(dpy, err->error_code, text, sizeof(text));

    if(xlib_error_log){
        mc_fmt(xlib_error_log, "[WM][XLIB] X error: %s (request %u.%u, resource 0x%lx)\n",
            text, (unsigned)err->request_code, (unsigned)err->minor_code,
            (unsigned long)err->resourceid);
    }

    return 0;
}

static MC_Error init(struct MC_TargetWM *wm, MC_Stream *log, MC_Alloc *arena){
    wm->log = log;
    wm->arena = arena;

    xlib_error_log = log;
    XSetErrorHandler(handle_x_error);

    wm->dpy = XOpenDisplay(NULL);
    if(wm->dpy == NULL){
        LOG("Could not open default display");
        return MCE_NOT_SUPPORTED;
    }

    wm->wm_protocols                = XInternAtom(wm->dpy, "WM_PROTOCOLS", False);
    wm->wm_delete_window            = XInternAtom(wm->dpy, "WM_DELETE_WINDOW", False);
    wm->net_wm_state                = XInternAtom(wm->dpy, "_NET_WM_STATE", False);
    wm->net_wm_state_fullscreen     = XInternAtom(wm->dpy, "_NET_WM_STATE_FULLSCREEN", False);
    wm->net_wm_state_maximized_horz = XInternAtom(wm->dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    wm->net_wm_state_maximized_vert = XInternAtom(wm->dpy, "_NET_WM_STATE_MAXIMIZED_VERT", False);
    wm->net_wm_state_hidden         = XInternAtom(wm->dpy, "_NET_WM_STATE_HIDDEN", False);

    wm->clipboard           = XInternAtom(wm->dpy, "CLIPBOARD", False);
    wm->utf8_string         = XInternAtom(wm->dpy, "UTF8_STRING", False);
    wm->selection_property  = XInternAtom(wm->dpy, "MC_SELECTION", False);

    return MCE_OK;
}

static void destroy(struct MC_TargetWM *wm){
    XCloseDisplay(wm->dpy);
}

static MC_Error init_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WindowParameters *params){
    int screen = DefaultScreen(wm->dpy);
    Window root = RootWindow(wm->dpy, screen);

    XSetWindowAttributes attributes;
    attributes.background_pixmap = None;

    window->window_id = XCreateWindow(wm->dpy, root, 0, 0, 800, 600, 0,
        DefaultDepth(wm->dpy, screen), CopyFromParent, DefaultVisual(wm->dpy, screen), CWBackPixmap, &attributes);

    params->identity = (uint64_t)window->window_id;

    XSelectInput(wm->dpy, window->window_id,
        KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask
        | LeaveWindowMask | PointerMotionMask | ButtonMotionMask | KeymapStateMask | ExposureMask
        | VisibilityChangeMask | StructureNotifyMask | SubstructureNotifyMask | FocusChangeMask
        | PropertyChangeMask);

    XSetWMProtocols(wm->dpy, window->window_id, &wm->wm_delete_window, 1);

    XMapWindow(wm->dpy, window->window_id);

    return MCE_OK;
}

static void destroy_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window){
    XDestroyWindow(wm->dpy, window->window_id);
}

static MC_Error create_window_graphic(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, struct MC_Graphics **g){
    return mc_xlib_graphics_init(g, wm->dpy, window->window_id);
}

static MC_Error set_window_title(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Str title){
    char buffer[256];
    size_t len = MIN(mc_str_len(title), sizeof(buffer) - 1);
    memcpy(buffer, title.beg, len);
    buffer[len] = '\0';

    XStoreName(wm->dpy, window->window_id, buffer);
    return MCE_OK;
}

static MC_Error set_window_rect(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WMArea area, MC_Rect2IU rect){
    (void)area;
    XMoveResizeWindow(wm->dpy, window->window_id, rect.x, rect.y, rect.width, rect.height);
    return MCE_OK;
}

static MC_Error get_window_rect(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WMArea area, MC_Rect2IU *rect){
    (void)area;

    XWindowAttributes attr;
    if(XGetWindowAttributes(wm->dpy, window->window_id, &attr) == 0){
        return MCE_UNKNOWN;
    }

    int x = 0;
    int y = 0;
    Window child;
    XTranslateCoordinates(wm->dpy, window->window_id,
        RootWindow(wm->dpy, DefaultScreen(wm->dpy)), 0, 0, &x, &y, &child);

    *rect = (MC_Rect2IU){ .x = x, .y = y, .width = (unsigned)attr.width, .height = (unsigned)attr.height };
    return MCE_OK;
}

static MC_Error get_window_title(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, char *utf8, size_t cap, size_t *len){
    char *name = NULL;
    if(XFetchName(wm->dpy, window->window_id, &name) == 0 || name == NULL){
        if(cap > 0){
            utf8[0] = '\0';
        }
        *len = 0;
        return MCE_OK;
    }

    size_t n = strlen(name);
    if(cap > 0){
        if(n > cap - 1){
            n = cap - 1;
        }
        memcpy(utf8, name, n);
        utf8[n] = '\0';
    }

    *len = n;
    XFree(name);
    return MCE_OK;
}

static MC_Error get_window_state(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WMWindowState *state){
    *state = read_window_state(wm, window->window_id);
    return MCE_OK;
}

static MC_Error focus_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window){
    XRaiseWindow(wm->dpy, window->window_id);
    XSetInputFocus(wm->dpy, window->window_id, RevertToParent, CurrentTime);
    XFlush(wm->dpy);
    return MCE_OK;
}

static MC_Error request_events(struct MC_TargetWM *wm, MC_WMEvents events){
    (void)wm;
    (void)events;
    return MCE_OK;
}

static MC_Error get_focused_window(struct MC_TargetWM *wm, uint64_t *identity){
    (void)wm;
    (void)identity;
    return MCE_NOT_SUPPORTED;
}

static MC_Error get_hovered_window(struct MC_TargetWM *wm, uint64_t *identity){
    (void)wm;
    (void)identity;
    return MCE_NOT_SUPPORTED;
}

static MC_Error get_all_windows(struct MC_TargetWM *wm, uint64_t **identities, size_t *count){
    (void)wm;
    (void)identities;
    (void)count;
    return MCE_NOT_SUPPORTED;
}

static MC_Error resolve_temporary_identity(struct MC_TargetWM *wm, uint64_t identity, MC_TargetResolvedIdentity *out){
    (void)wm;
    (void)identity;
    (void)out;
    return MCE_NOT_FOUND;
}

static void heartbeat(struct MC_TargetWM *wm){
    (void)wm;
}

static void destroy_foreign_window(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window){
    (void)wm;
    (void)window;
}

static MC_Error close_foreign_window(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window){
    (void)wm;
    (void)window;
    return MCE_NOT_SUPPORTED;
}

static MC_Error set_foreign_window_title(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_Str title){
    (void)wm;
    (void)window;
    (void)title;
    return MCE_NOT_SUPPORTED;
}

static MC_Error set_foreign_window_state(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_WMWindowState state){
    (void)wm;
    (void)window;
    (void)state;
    return MCE_NOT_SUPPORTED;
}

static MC_Error set_foreign_window_rect(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_WMArea area, MC_Rect2IU rect){
    (void)wm;
    (void)window;
    (void)area;
    (void)rect;
    return MCE_NOT_SUPPORTED;
}

static MC_Error focus_foreign_window(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window){
    (void)wm;
    (void)window;
    return MCE_NOT_SUPPORTED;
}

static MC_Error get_foreign_window_title(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, char *utf8, size_t cap, size_t *len){
    (void)wm;
    (void)window;
    (void)utf8;
    (void)cap;
    (void)len;
    return MCE_NOT_SUPPORTED;
}

static MC_Error get_foreign_window_state(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_WMWindowState *state){
    (void)wm;
    (void)window;
    (void)state;
    return MCE_NOT_SUPPORTED;
}

static MC_Error get_foreign_window_rect(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_WMArea area, MC_Rect2IU *rect){
    (void)wm;
    (void)window;
    (void)area;
    (void)rect;
    return MCE_NOT_SUPPORTED;
}

static MC_Error is_foreign_window_system(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, bool *out){
    (void)wm;
    (void)window;
    (void)out;
    return MCE_NOT_SUPPORTED;
}

static void net_wm_state(struct MC_TargetWM *wm, Window win, long action, Atom first, Atom second){
    XEvent ev = {0};
    ev.xclient.type = ClientMessage;
    ev.xclient.window = win;
    ev.xclient.message_type = wm->net_wm_state;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = action;
    ev.xclient.data.l[1] = first;
    ev.xclient.data.l[2] = second;
    ev.xclient.data.l[3] = 1;

    Window root = RootWindow(wm->dpy, DefaultScreen(wm->dpy));
    XSendEvent(wm->dpy, root, False, SubstructureRedirectMask | SubstructureNotifyMask, &ev);
}

static MC_Error set_window_state(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WMWindowState state){
    enum { NET_WM_STATE_REMOVE, NET_WM_STATE_ADD };

    Window win = window->window_id;

    switch(state){
    case MC_WM_WINDOW_STATE_NORMAL:
        XMapWindow(wm->dpy, win);
        net_wm_state(wm, win, NET_WM_STATE_REMOVE, wm->net_wm_state_fullscreen, 0);
        net_wm_state(wm, win, NET_WM_STATE_REMOVE,
            wm->net_wm_state_maximized_horz, wm->net_wm_state_maximized_vert);
        break;
    case MC_WM_WINDOW_STATE_MINIMIZED:
        XIconifyWindow(wm->dpy, win, DefaultScreen(wm->dpy));
        break;
    case MC_WM_WINDOW_STATE_MAXIMIZED:
        XMapWindow(wm->dpy, win);
        net_wm_state(wm, win, NET_WM_STATE_REMOVE, wm->net_wm_state_fullscreen, 0);
        net_wm_state(wm, win, NET_WM_STATE_ADD,
            wm->net_wm_state_maximized_horz, wm->net_wm_state_maximized_vert);
        break;
    case MC_WM_WINDOW_STATE_FULLSCREEN:
        XMapWindow(wm->dpy, win);
        net_wm_state(wm, win, NET_WM_STATE_ADD, wm->net_wm_state_fullscreen, 0);
        break;
    default:
        LOG("set_window_state: unsupported state %u", (unsigned)state);
        return MCE_INVALID_INPUT;
    }

    XFlush(wm->dpy);
    return MCE_OK;
}

static MC_WMWindowState read_window_state(struct MC_TargetWM *wm, Window win){
    Atom type;
    int format;
    unsigned long nitems;
    unsigned long remaining;
    unsigned char *bytes = NULL;

    if(XGetWindowProperty(wm->dpy, win, wm->net_wm_state, 0, 32, False,
        XA_ATOM, &type, &format, &nitems, &remaining, &bytes) != Success || bytes == NULL){
        LOG("read_window_state: could not read _NET_WM_STATE");
        return MC_WM_WINDOW_STATE_NORMAL;
    }

    Atom *atoms = (Atom*)bytes;
    bool fullscreen = false;
    bool hidden = false;
    bool maximized_horz = false;
    bool maximized_vert = false;

    for(unsigned long i = 0; i < nitems; i++){
        if(atoms[i] == wm->net_wm_state_fullscreen){
            fullscreen = true;
        }
        else if(atoms[i] == wm->net_wm_state_hidden){
            hidden = true;
        }
        else if(atoms[i] == wm->net_wm_state_maximized_horz){
            maximized_horz = true;
        }
        else if(atoms[i] == wm->net_wm_state_maximized_vert){
            maximized_vert = true;
        }
    }

    XFree(bytes);

    if(fullscreen){
        return MC_WM_WINDOW_STATE_FULLSCREEN;
    }

    if(hidden){
        return MC_WM_WINDOW_STATE_MINIMIZED;
    }

    if(maximized_horz && maximized_vert){
        return MC_WM_WINDOW_STATE_MAXIMIZED;
    }

    return MC_WM_WINDOW_STATE_NORMAL;
}

static bool poll_event(struct MC_TargetWM *wm, struct MC_TargetWMEvent *event){
    if(XPending(wm->dpy) == 0){
        return false;
    }

    XNextEvent(wm->dpy, &event->event);
    return true;
}

static unsigned translate_event(struct MC_TargetWM *wm, const struct MC_TargetWMEvent *event, MC_TargetIndication indications[MC_WM_MAX_INDICATIONS_PER_EVENT]){
    (void)wm;
    const XEvent *e = &event->event;

    switch (e->type){
    case ConfigureNotify:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_WINDOW_MOVED,
            .as.window_moved = {
                .window = get_window(wm, e->xany.window),
                .new_position = {
                    .x = e->xconfigure.x,
                    .y = e->xconfigure.y,
                },
            }
        };

        indications[1] = (MC_TargetIndication){
            .type = MC_WMIND_WINDOW_RESIZED,
            .as.window_moved = {
                .window = get_window(wm, e->xany.window),
                .new_position = {
                    .x = e->xconfigure.width,
                    .y = e->xconfigure.height,
                },
            }
        };

        return 2;
    case ButtonPress:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_MOUSE_DOWN,
            .as.mouse_down = {
                .window = get_window(wm, e->xany.window),
                .button = get_mouse_button(e->xbutton.button)
            }
        };

        if(e->xbutton.button == Button2){
            XConvertSelection(wm->dpy, XA_PRIMARY, wm->utf8_string,
                wm->selection_property, e->xbutton.window, e->xbutton.time);
        }

        return 1;
    case ButtonRelease:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_MOUSE_UP,
            .as.mouse_up = {
                .window = get_window(wm, e->xany.window),
                .button = get_mouse_button(e->xbutton.button)
            }
        };

        return 1;
    case MotionNotify:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_MOUSE_MOVED,
            .as.mouse_moved = {
                .window = get_window(wm, e->xany.window),
            }
        };

        return 1;
    case EnterNotify:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_MOUSE_ENTER,
            .as.mouse_enter = {
                .window = get_window(wm, e->xany.window),
            }
        };

        return 1;
    case LeaveNotify:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_MOUSE_LEAVE,
            .as.mouse_enter = {
                .window = get_window(wm, e->xany.window),
            }
        };

        return 1;
    case Expose:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_WINDOW_REDRAW_REQUESTED,
            .as.redraw_requested = {
                .window = get_window(wm, e->xany.window),
            }
        };

        return 1;
    case PropertyNotify:
        if(e->xproperty.atom == wm->net_wm_state){
            indications[0] = (MC_TargetIndication){
                .type = MC_WMIND_WINDOW_STATE_CHANGED,
                .as.window_state_changed = {
                    .window = get_window(wm, e->xany.window),
                    .state = read_window_state(wm, e->xproperty.window),
                }
            };

            return 1;
        }

        return 0;
    case ClientMessage:
        if((Atom)e->xclient.message_type == wm->wm_protocols
            && (Atom)e->xclient.data.l[0] == wm->wm_delete_window){
            indications[0] = (MC_TargetIndication){
                .type = MC_WMIND_WINDOW_CLOSE_REQUESTED,
                .as.window_close_requested = {
                    .window = get_window(wm, e->xany.window),
                }
            };

            return 1;
        }

        return 0;
    case FocusIn:
        if(e->xfocus.mode == NotifyGrab || e->xfocus.mode == NotifyUngrab
            || e->xfocus.detail == NotifyPointer){
            return 0;
        }

        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_FOCUS_GAINED,
            .as.focus_gained = {
                .window = get_window(wm, e->xany.window),
            }
        };

        return 1;
    case FocusOut:
        if(e->xfocus.mode == NotifyGrab || e->xfocus.mode == NotifyUngrab
            || e->xfocus.detail == NotifyPointer){
            return 0;
        }

        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_FOCUS_LOST,
            .as.focus_lost = {
                .window = get_window(wm, e->xany.window),
            }
        };

        return 1;
    case SelectionNotify: {
        Atom property = e->xselection.property;
        if(property == None){
            LOG("paste: selection conversion failed (no owner or unsupported target)");
            return 0;
        }

        Window requestor = e->xselection.requestor;

        Atom type;
        int format;
        unsigned long nitems;
        unsigned long remaining;
        unsigned char *bytes = NULL;

        int status = XGetWindowProperty(wm->dpy, requestor, property, 0, ~0L, True,
            AnyPropertyType, &type, &format, &nitems, &remaining, &bytes);

        if(status != Success || bytes == NULL){
            LOG("paste: could not read selection property (status %d)", status);
            if(bytes){
                XFree(bytes);
            }

            return 0;
        }

        if(format != 8 || (type != wm->utf8_string && type != XA_STRING)){
            LOG("paste: selection is not text (format %d, type %lu)", format, (unsigned long)type);
            XFree(bytes);
            return 0;
        }

        char *text;
        if(mc_alloc(wm->arena, nitems + 1, (void**)&text) != MCE_OK){
            LOG("paste: failed to allocate %lu bytes for pasted text", (unsigned long)nitems + 1);
            XFree(bytes);
            return 0;
        }

        memcpy(text, bytes, nitems);
        text[nitems] = '\0';
        XFree(bytes);

        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_PASTE_TEXT,
            .as.paste_text = {
                .window = get_window(wm, requestor),
                .text = {.beg = text, .end = text + nitems},
            }
        };

        return 1;
    }
    case KeyPress: {
        unsigned n = 0;

        indications[n++] = (MC_TargetIndication){
            .type = MC_WMIND_KEY_DOWN,
            .as.key_down = {
                .key = get_key(&e->xkey),
            }
        };

        char buf[MC_WM_TEXT_INPUT_CAP];
        KeySym sym = NoSymbol;
        XKeyEvent ke = e->xkey;
        int len = XLookupString(&ke, buf, sizeof(buf) - 1, &sym, NULL);

        if(len > 0 && (unsigned char)buf[0] >= 0x20 && (unsigned char)buf[0] != 0x7F){
            indications[n] = (MC_TargetIndication){
                .type = MC_WMIND_TEXT_INPUT,
                .as.text_input = {
                    .window = get_window(wm, e->xany.window),
                }
            };

            memcpy(indications[n].as.text_input.utf8, buf, (size_t)len);
            indications[n].as.text_input.utf8[len] = '\0';
            n++;
        }

        if((sym == XK_v && (e->xkey.state & ControlMask))
            || (sym == XK_Insert && (e->xkey.state & ShiftMask))){
            XConvertSelection(wm->dpy, wm->clipboard, wm->utf8_string,
                wm->selection_property, e->xkey.window, e->xkey.time);
        }

        return n;
    }
    case KeyRelease:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_KEY_UP,
            .as.key_up = {
                .key = get_key(&e->xkey),
            }
        };

        return 1;
    default: return 0;
    }
}

static MC_MouseButton get_mouse_button(int x11_mouse_button){
    switch (x11_mouse_button){
    case Button1: return MC_MOUSE_LEFT;
    case Button2: return MC_MOUSE_MIDDLE;
    case Button3: return MC_MOUSE_RIGHT;
    case Button4: return MC_MOUSE_BUTTON4;
    case Button5: return MC_MOUSE_BUTTON5;
    default: return MC_MOUSE_UNKNOWN;
    }
}

static MC_Key get_key(const XKeyEvent *xk){
    XKeyEvent mut_xk = *xk;
    KeySym sym = XLookupKeysym(&mut_xk, 0);
    switch (sym){
    #define MC_XKEY(XNAME, NAME) case XK_##XNAME: return MC_KEY_##NAME;
    MC_X11_ITER_KEYS()
    #undef MC_XKEY
    default:
        mc_fmt(MC_STDERR, "unknown key %X\n", (unsigned)sym);
        return MC_KEY_UNKNOWN;
    }
}

static struct MC_TargetWMWindow *get_window(struct MC_TargetWM *wm, Window xid){
    (void)wm;
    (void)xid;
    return NULL;
}

