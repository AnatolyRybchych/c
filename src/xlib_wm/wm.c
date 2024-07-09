#include <mc/xlib_wm/wm.h>

#define LOG(FMT, ...) mc_fmt(wm->log, "[WM][XLIB] " FMT "\n", ##__VA_ARGS__)

static MC_Error init(struct MC_TargetWM *wm, MC_Stream *log);
static void destroy(struct MC_TargetWM *wm);
static MC_Error init_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window);
static void destroy_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window);
static bool poll_event(struct MC_TargetWM *wm, struct MC_TargetWMEvent *event);
static unsigned translate_event(struct MC_TargetWM *wm, const struct MC_TargetWMEvent *event, MC_TargetIndication indications[16]);

static MC_WMVtab vtab = {
    .name = "X11",
    .wm_size = sizeof(struct MC_TargetWM),
    .window_size = sizeof(struct MC_TargetWMWindow),
    .event_size = sizeof(struct MC_TargetWMEvent),

    .init = init,
    .destroy = destroy,

    .init_window = init_window,
    .destroy_window = destroy_window,

    .poll_event = poll_event,
    .translate_event = translate_event,
};

const MC_WMVtab *mc_xlib_wm_vtab = &vtab;

static MC_Error init(struct MC_TargetWM *wm, MC_Stream *log){
    wm->log = log;

    wm->dpy = XOpenDisplay(NULL);
    if(wm->dpy == NULL){
        LOG("Could not open default display");
        return MCE_NOT_SUPPORTED;
    }

    return MCE_OK;
}

static void destroy(struct MC_TargetWM *wm){
    XCloseDisplay(wm->dpy);
}

static MC_Error init_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window){
    int screen = DefaultScreen(wm->dpy);
    Window root = RootWindow(wm->dpy, screen);

    window->window_id = XCreateSimpleWindow(wm->dpy, root, 0, 0, 800, 600, 0,
        BlackPixel(wm->dpy, screen), WhitePixel(wm->dpy, screen));

    XSelectInput(wm->dpy, window->window_id,
        KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask
        | LeaveWindowMask | PointerMotionMask | PointerMotionHintMask | ButtonMotionMask
        | KeymapStateMask | ExposureMask | VisibilityChangeMask | StructureNotifyMask | ResizeRedirectMask
        | SubstructureNotifyMask | SubstructureRedirectMask | FocusChangeMask | PropertyChangeMask
        | ColormapChangeMask | OwnerGrabButtonMask);

    XMapWindow(wm->dpy, window->window_id);

    return MCE_OK;
}

static void destroy_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window){
    XDestroyWindow(wm->dpy, window->window_id);
}

static bool poll_event(struct MC_TargetWM *wm, struct MC_TargetWMEvent *event){
    if(XPending(wm->dpy) == 0){
        return false;
    }

    XNextEvent(wm->dpy, &event->event);
    return true;
}

static unsigned translate_event(struct MC_TargetWM *wm, const struct MC_TargetWMEvent *event, MC_TargetIndication indications[16]){
    (void)wm;
    (void)event;
    (void)indications;
    return 0;
}
