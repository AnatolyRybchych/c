#include <mc/xlib_wm/wm.h>
#include <mc/xlib_wm/graphics.h>
#include <mc/xlib_wm/key.h>
#include <mc/os/file.h>

#include <X11/keysym.h>

#define LOG(FMT, ...) mc_fmt(wm->log, "[WM][XLIB] " FMT "\n", ##__VA_ARGS__)

struct MC_TargetWM{
    MC_Stream *log;
    Display *dpy;

    Atom wm_protocols;
    Atom wm_delete_window;
    Atom net_wm_state;
    Atom net_wm_state_fullscreen;
    Atom net_wm_state_maximized_horz;
    Atom net_wm_state_maximized_vert;
};

struct MC_TargetWMWindow{
    Window window_id;
};

struct MC_TargetWMEvent{
    XEvent event;
};

static MC_Error init(struct MC_TargetWM *wm, MC_Stream *log);
static void destroy(struct MC_TargetWM *wm);
static MC_Error init_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window);
static void destroy_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window);
static MC_Error create_window_graphic(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, struct MC_Graphics **g);
static MC_Error set_window_title(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Str title);
static MC_Error set_window_size(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Size2U size);
static MC_Error set_window_state(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WMWindowState state);
static bool poll_event(struct MC_TargetWM *wm, struct MC_TargetWMEvent *event);
static unsigned translate_event(struct MC_TargetWM *wm, const struct MC_TargetWMEvent *event, MC_TargetIndication indications[MC_WM_MAX_INDICATIONS_PER_EVENT]);

static MC_MouseButton get_mouse_button(int x11_mouse_button);
static MC_Key get_key(const XKeyEvent *xk);

static struct MC_TargetWMWindow *get_window(struct MC_TargetWM *wm, Window xid);

static MC_WMVtab vtab = {
    .name = "X11",
    .wm_size = sizeof(struct MC_TargetWM),
    .window_size = sizeof(struct MC_TargetWMWindow),
    .event_size = sizeof(struct MC_TargetWMEvent),

    .init = init,
    .destroy = destroy,

    .init_window = init_window,
    .destroy_window = destroy_window,
    .create_window_graphic = create_window_graphic,

    .set_window_title = set_window_title,
    .set_window_size = set_window_size,
    .set_window_state = set_window_state,

    .poll_event = poll_event,
    .translate_event = translate_event,
};

const MC_WMVtab *mc_xlib_wm_vtab = &vtab;

Display *mc_wm_xlib_get_display(MC_TargetWM *wm){
    return wm->dpy;
}

Window mc_wm_xlib_window_get_xid(MC_TargetWMWindow *window){
    return window->window_id;
}

static MC_Error init(struct MC_TargetWM *wm, MC_Stream *log){
    wm->log = log;

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

    return MCE_OK;
}

static void destroy(struct MC_TargetWM *wm){
    XCloseDisplay(wm->dpy);
}

static MC_Error init_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window){
    int screen = DefaultScreen(wm->dpy);
    Window root = RootWindow(wm->dpy, screen);

    XSetWindowAttributes attributes;
    attributes.background_pixmap = None;

    window->window_id = XCreateWindow(wm->dpy, root, 0, 0, 800, 600, 0,
        DefaultDepth(wm->dpy, screen), CopyFromParent, DefaultVisual(wm->dpy, screen), CWBackPixmap, &attributes);

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

static MC_Error set_window_size(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Size2U size){
    XResizeWindow(wm->dpy, window->window_id, size.width, size.height);
    return MCE_OK;
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
        return MCE_INVALID_INPUT;
    }

    XFlush(wm->dpy);
    return MCE_OK;
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
    case KeyPress:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_KEY_DOWN,
            .as.key_down = {
                .key = get_key(&e->xkey),
            }
        };

        return 1;
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

